#include "obj_file_loading.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <array>
#include <vector>
#include <stdexcept>

#include <glm/gtx/string_cast.hpp>

using VertexFingerprint = std::array<uint64_t, 3>;

ObjMesh loadOBJ(const fs::path& aObjPath) {
	if (!fs::exists(aObjPath) || !fs::is_regular_file(aObjPath)) {
		throw std::runtime_error("File does not exist or is not a regular file: " + aObjPath.string());
	}

	std::ifstream file(aObjPath);

	std::map<VertexFingerprint, int> vertexIndices;
	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> texCoords;
	std::vector<glm::vec3> normals;
	ObjMesh mesh;

	std::string line;
	uint64_t lineNumber = 0;
	while (std::getline(file, line)) {
		++lineNumber;
		std::istringstream iss(line);
		std::string prefix;
		iss >> prefix;

		if (prefix == "v") {
			glm::vec3 position;
			if (!(iss >> position.x >> position.y >> position.z)) {
				throw std::runtime_error(
						"Invalid vertex position format in file: "
						+ aObjPath.string() + " on line " + std::to_string(lineNumber));
			}
			positions.push_back(position);
		} else if (prefix == "vt") {
			glm::vec2 texCoord;
			if (!(iss >> texCoord.x >> texCoord.y)) {
				throw std::runtime_error(
						"Invalid texture coordinate format in file: "
						+ aObjPath.string() + " on line " + std::to_string(lineNumber));
			}
			texCoords.push_back(texCoord);
		} else if (prefix == "vn") {
			glm::vec3 normal;
			if (!(iss >> normal.x >> normal.y >> normal.z)) {
				throw std::runtime_error(
						"Invalid normal format in file: "
						+ aObjPath.string() + " on line " + std::to_string(lineNumber));
			}
			normals.push_back(normal);
		} else if (prefix == "f") {
			std::vector<VertexFingerprint> faceVertices;
			std::string token;
			while (iss >> token) {
				uint64_t vertIndex = 0, texIndex = 0, normIndex = 0;
				size_t firstSlash = token.find('/');
				if (firstSlash == std::string::npos) {
					vertIndex = std::stoull(token);
				} else {
					vertIndex = std::stoull(token.substr(0, firstSlash));
					size_t secondSlash = token.find('/', firstSlash + 1);
					if (secondSlash == std::string::npos) {
						std::string texStr = token.substr(firstSlash + 1);
						if (!texStr.empty()) texIndex = std::stoull(texStr);
					} else {
						std::string texStr = token.substr(firstSlash + 1, secondSlash - (firstSlash + 1));
						if (!texStr.empty()) texIndex = std::stoull(texStr);
						std::string normStr = token.substr(secondSlash + 1);
						if (!normStr.empty()) normIndex = std::stoull(normStr);
					}
				}

				if (texIndex == 0) {
					if (texCoords.empty()) {
						texCoords.push_back(glm::vec2(0.0f, 0.0f));
					}
					texIndex = 1;
				}
				if (normIndex == 0) {
					if (normals.empty()) {
						normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
					}
					normIndex = 1;
				}

				if (vertIndex == 0 || vertIndex > positions.size() || 
					texIndex == 0 || texIndex > texCoords.size() || 
					normIndex == 0 || normIndex > normals.size()) {
					throw std::runtime_error(
						"Face index out of bounds in file: "
						+ aObjPath.string() + " on line " + std::to_string(lineNumber));
				}

				faceVertices.push_back({ vertIndex - 1, texIndex - 1, normIndex - 1 });
			}

			// Fan triangulation for quads and n-gons
			if (faceVertices.size() >= 3) {
				for (size_t i = 1; i + 1 < faceVertices.size(); ++i) {
					std::array<VertexFingerprint, 3> tri = { faceVertices[0], faceVertices[i], faceVertices[i + 1] };
					for (const auto& fp : tri) {
						auto it = vertexIndices.find(fp);
						if (it == vertexIndices.end()) {
							VertexNormTex vertex = {
								positions[fp[0]],
								normals[fp[2]],
								texCoords[fp[1]]
							};
							mesh.vertices.push_back(vertex);
							unsigned newIdx = static_cast<unsigned>(mesh.vertices.size() - 1);
							vertexIndices[fp] = newIdx;
							mesh.indices.push_back(newIdx);
						} else {
							mesh.indices.push_back(it->second);
						}
					}
				}
			}
		}
	}

	if (mesh.vertices.empty() || mesh.indices.empty()) {
		throw std::runtime_error("Empty mesh or missing data in file: " + aObjPath.string());
	}

	return mesh;
}
