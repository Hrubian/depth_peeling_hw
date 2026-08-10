#pragma once

#include <vector>
#include <memory>
#include <iostream>

#include "camera.hpp"
#include "ogl_material_factory.hpp"
#include "ogl_geometry_factory.hpp"
#include "ogl_resource.hpp"

// Custom Framebuffer supporting both Color (RGBA16F) and Depth (DEPTH_COMPONENT32F) Textures
class PeelFramebuffer {
public:
	PeelFramebuffer(int aWidth, int aHeight)
		: mWidth(aWidth)
		, mHeight(aHeight)
		, mFramebuffer(createFramebuffer())
		, mColorTexture(createTexture())
		, mDepthTexture(createTexture())
	{
		bind();

		// Color Texture Attachment (RGBA16F)
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, mColorTexture.get()));
		GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, nullptr));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTexture.get(), 0));

		// Depth Texture Attachment (DEPTH_COMPONENT32F)
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, mDepthTexture.get()));
		GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, mWidth, mHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mDepthTexture.get(), 0));

		GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		GL_CHECK(glDrawBuffers(1, drawBuffers));

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			throw OpenGLError("PeelFramebuffer incomplete!");
		}

		unbind();
	}

	void bind() const {
		GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer.get()));
	}

	void unbind() const {
		GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}

	GLuint getColorTexture() const { return mColorTexture.get(); }
	GLuint getDepthTexture() const { return mDepthTexture.get(); }

private:
	int mWidth;
	int mHeight;
	OpenGLResource mFramebuffer;
	OpenGLResource mColorTexture;
	OpenGLResource mDepthTexture;
};

// Quad renderer for full-screen compositing passes
class QuadRenderer {
public:
	QuadRenderer()
		: mQuad(generateQuadTex())
	{}

	void render(const OGLShaderProgram &aShaderProgram, MaterialParameterValues &aParameters) const {
		aShaderProgram.use();
		aShaderProgram.setMaterialParameters(aParameters, MaterialParameterValues());
		GL_CHECK(glBindVertexArray(mQuad.vao.get()));
		GL_CHECK(glDrawElements(mQuad.mode, mQuad.indexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(0)));
	}

	IndexedBuffer mQuad;
};

constexpr int MAX_PEELING_PASSES = 8;

class Renderer {
public:
	Renderer(OGLMaterialFactory &aMaterialFactory, int aNumPeelingPasses = 3)
		: mMaterialFactory(aMaterialFactory)
		, mNumPeelingPasses(glm::clamp(aNumPeelingPasses, 1, MAX_PEELING_PASSES))
	{
		mBlinnPhongShader = std::static_pointer_cast<OGLShaderProgram>(
				mMaterialFactory.getShaderProgram("blinn_phong"));
		mCompositeShader = std::static_pointer_cast<OGLShaderProgram>(
				mMaterialFactory.getShaderProgram("composite"));
		mPassthroughShader = std::static_pointer_cast<OGLShaderProgram>(
				mMaterialFactory.getShaderProgram("passthrough"));
	}

	void initialize(int aWidth, int aHeight) {
		mWidth = aWidth;
		mHeight = aHeight;
		recreateFramebuffers();
	}

	void setNumPeelingPasses(int aNumPeelingPasses) {
		int clamped = glm::clamp(aNumPeelingPasses, 1, MAX_PEELING_PASSES);
		if (clamped != mNumPeelingPasses) {
			mNumPeelingPasses = clamped;
			recreateFramebuffers();
		}
	}

	int getNumPeelingPasses() const {
		return mNumPeelingPasses;
	}

	template<typename TScene, typename TCamera, typename TLight>
	void peelPass(const TScene &aScene, const TCamera &aCamera, const TLight &aLight, int aPassIndex) {
		GL_CHECK(glEnable(GL_DEPTH_TEST));
		GL_CHECK(glDepthFunc(GL_LESS));
		GL_CHECK(glDisable(GL_BLEND)); // Disable hardware blending during offscreen peeling passes
		GL_CHECK(glEnable(GL_CULL_FACE)); // Enable culling back faces so each 3D mesh is 1 depth layer per building
		GL_CHECK(glCullFace(GL_BACK));
		GL_CHECK(glFrontFace(GL_CCW));
		GL_CHECK(glViewport(0, 0, mWidth, mHeight));

		mPeelFramebuffers[aPassIndex]->bind();
		GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

		// If pass index > 0, bind previous pass's depth texture to unit 1 for depth peeling comparison
		if (aPassIndex > 0) {
			GL_CHECK(glActiveTexture(GL_TEXTURE1));
			GL_CHECK(glBindTexture(GL_TEXTURE_2D, mPeelFramebuffers[aPassIndex - 1]->getDepthTexture()));
		}

		auto projection = aCamera.getProjectionMatrix();
		auto view = aCamera.getViewMatrix();

		MaterialParameterValues fallbackParameters;
		fallbackParameters["u_projMat"] = projection;
		fallbackParameters["u_viewMat"] = view;
		fallbackParameters["u_viewPos"] = aCamera.getPosition();
		fallbackParameters["u_lightPos"] = aLight.getPosition();
		fallbackParameters["u_solidColor"] = glm::vec4(0, 0, 0, 1);
		fallbackParameters["u_near"] = aCamera.near();
		fallbackParameters["u_far"] = aCamera.far();
		fallbackParameters["u_screenSize"] = glm::vec2(mWidth, mHeight);
		fallbackParameters["u_usePeeling"] = (aPassIndex > 0);

		std::vector<RenderData> renderData;
		RenderOptions renderOptions = {"solid"};
		for (const auto &object : aScene.getObjects()) {
			auto data = object.getRenderData(renderOptions);
			if (data) {
				renderData.push_back(data.value());
			}
		}

		mBlinnPhongShader->use();
		for (const auto &data : renderData) {
			const glm::mat4 modelMat = data.modelMat;
			const MaterialParameters &params = data.mMaterialParams;
			const OGLShaderProgram &shaderProgram = static_cast<const OGLShaderProgram &>(data.mShaderProgram);
			const OGLGeometry &geometry = static_cast<const OGLGeometry&>(data.mGeometry);

			fallbackParameters["u_modelMat"] = modelMat;
			fallbackParameters["u_normalMat"] = glm::mat3(modelMat);

			shaderProgram.use();
			shaderProgram.setMaterialParameters(params.mParameterValues, fallbackParameters);

			geometry.bind();
			geometry.draw();
		}

		mPeelFramebuffers[aPassIndex]->unbind();
	}

	void compositePass() {
		GL_CHECK(glDisable(GL_DEPTH_TEST));
		GL_CHECK(glDisable(GL_CULL_FACE));
		GL_CHECK(glEnable(GL_BLEND));
		GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

		mCompositeShader->use();

		// Bind each peel layer's color texture to texture unit 0..mNumPeelingPasses-1
		for (int i = 0; i < mNumPeelingPasses; ++i) {
			GL_CHECK(glActiveTexture(GL_TEXTURE0 + i));
			GL_CHECK(glBindTexture(GL_TEXTURE_2D, mPeelFramebuffers[i]->getColorTexture()));
		}

		GL_CHECK(glActiveTexture(GL_TEXTURE0));

		mCompositingParameters["u_numLayers"] = mNumPeelingPasses;
		mCompositeShader->setMaterialParameters(mCompositingParameters);

		mQuadRenderer.render(*mCompositeShader, mCompositingParameters);
	}

	template<typename TScene, typename TCamera, typename TLight>
	void render(const TScene &aScene, const TCamera &aCamera, const TLight &aLight, RenderOptions aRenderOptions) {
		for (int i = 0; i < mNumPeelingPasses; ++i) {
			peelPass(aScene, aCamera, aLight, i);
		}

		compositePass();
	}

private:
	void recreateFramebuffers() {
		if (mWidth <= 0 || mHeight <= 0) return;
		mPeelFramebuffers.clear();
		mPeelFramebuffers.reserve(mNumPeelingPasses);
		for (int i = 0; i < mNumPeelingPasses; ++i) {
			mPeelFramebuffers.push_back(std::make_unique<PeelFramebuffer>(mWidth, mHeight));
		}
	}

	int mWidth = 100;
	int mHeight = 100;
	int mNumPeelingPasses;

	OGLMaterialFactory &mMaterialFactory;

	std::shared_ptr<OGLShaderProgram> mBlinnPhongShader;
	std::shared_ptr<OGLShaderProgram> mCompositeShader;
	std::shared_ptr<OGLShaderProgram> mPassthroughShader;

	std::vector<std::unique_ptr<PeelFramebuffer>> mPeelFramebuffers;

	MaterialParameterValues mCompositingParameters;
	QuadRenderer mQuadRenderer;
};
