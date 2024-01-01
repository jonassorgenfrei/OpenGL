#ifndef  MODEL_H
#define MODEL_H

#include <glad\glad.h>

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

#include "stb_image.h"
#include "mesh.h"
#include "shader_m.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

unsigned TextureFromFile(const char *path, const string &directory, bool gamma = false);

class Model
{
	public:
		/* Model Data */
		vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
		vector<Mesh> meshes;
		string directory;
		bool gammaCorrection;

		/* Functions */
		Model() {}

		// constructor, expects a filepath to a 3D model.
		Model(const char *path)
		{
			loadModel(path);
		}
		void Draw(Shader shader) {
			for (unsigned int i = 0; i < meshes.size(); i++)
				meshes[i].Draw(shader);
		};
	private:

		/* Functions */
		// loads a model with supported ASSIMP extensions from file and 
		// stores the resulting meshes in the meshes vector.
		void loadModel(string path)
		{
			//read file via ASSIMP
			Assimp::Importer import;
			//Importer Object
			const aiScene *scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
			//file path & post-processing options
			// aiProcess_Triangulate -> if model does not (entirely) consist of triangles it should
			//							transform all the model's primitive shapes to triangles
			// aiProcess_FlipUVs -> flips the texture coordinates on the y-axis where necessary during processing
			//						(most textures in OpenGL were reversed around the y-axis)
			// aiProcess_GenNormals -> creates normals for each vertex if the model didn't contain normal vectors
			// aiProcess_SpliLargeMeshes -> splits large meshes into smaller sub-meshes 
			//								(useful if rendering has max. number of vertices allowed)
			// aiProcess_OptimizeMeshes -> tries to join several meshes into one larger mesh, 
			//								reduces drawing calls for optimization
			// more: http://assimp.sourceforge.net/lib_html/postprocess_8h.html

			//check if scene and the root node are not null
			//check if returned data of flag ist incomplete
			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			{
				//return error
				cout << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
				return;
			}
			// retrieve the directory path of the filepath
			directory = path.substr(0, path.find_last_of('/'));
			
			// process ASSIMP's root node recursively
			processNode(scene->mRootNode, scene);
		};

		// processes a node in a recursive fashion. Processes each 
		// individual mesh located at the node and repeats this process on its children 
		// nodes (if any).
		void processNode(aiNode *node, const aiScene *scene) {
			//process all the current node's meshes (if any)
			for (unsigned int i = 0; i < node->mNumMeshes; i++)
			{
				// the node object only contains indices to index the actual objects in the scene. 
				// the scene contains all the data, node is just to keep stuff organized 
				// (like relations between nodes).
				aiMesh * mesh = scene->mMeshes[node->mMeshes[i]];
				meshes.push_back(processMesh(mesh, scene));
				//store in the meshes list/vector
			}
			// then do the same for each of its children
			for (unsigned int i = 0; i < node->mNumChildren; i++)
			{
				processNode(node->mChildren[i], scene);
				// iterate through all of the node's children and call the same processNode function for
				//each of the node's children
				//stops once a node no longer has any children
			}
		};

		Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
			// data to fill
			vector<Vertex> vertices;
			vector<unsigned int> indices;
			vector<Texture> textures;

			// Walk through each of the mesh's vertices
			for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			{
				
				Vertex vertex;

				// process vertex positions, normals and texture coordinates
				
				// we declare a placeholder vector since assimp uses its own vector 
				// class that doesn't directly convert to glm's vec3 class so we transfer 
				// the data to this placeholder glm::vec3 vector.
				glm::vec3 vector;
				// position
				if (mesh->mVertices) {
					vector.x = mesh->mVertices[i].x;
					vector.y = mesh->mVertices[i].y;
					vector.z = mesh->mVertices[i].z;
					vertex.Position = vector;
				}
				// normals
				if (mesh->mNormals) {
					vector.x = mesh->mNormals[i].x;
					vector.y = mesh->mNormals[i].y;
					vector.z = mesh->mNormals[i].z;
					vertex.Normal = vector;
				}

				// textures coordinates
				if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
				{
					glm::vec2 vec;
					// a vertex can contain up to 8 different texture coordinates. 
					// We thus make the assumption that we won't 
					// use models where a vertex can have multiple texture coordinates so 
					// we always take the first set (0).
					vec.x = mesh->mTextureCoords[0][i].x;
					vec.y = mesh->mTextureCoords[0][i].y;
					vertex.TexCoords = vec;
				}
				else
					vertex.TexCoords = glm::vec2(0.0f, 0.0f);

				// tangent
				if (mesh->mTangents) {
					vector.x = mesh->mTangents[i].x;
					vector.y = mesh->mTangents[i].y;
					vector.z = mesh->mTangents[i].z;
					vertex.Tangent = vector;
				}
				// bitangent
				if (mesh->mBitangents) {
					vector.x = mesh->mBitangents[i].x;
					vector.y = mesh->mBitangents[i].y;
					vector.z = mesh->mBitangents[i].z;
					vertex.Bitangent = vector;
				}
				vertices.push_back(vertex);
			}
			//process indices for faces (=> triangles)
			for (unsigned int i = 0; i < mesh->mNumFaces; i++)
			{
				aiFace face = mesh->mFaces[i];
				// retrieve all indices of the face and store them in the indices vector
				for (unsigned int j = 0; j < face.mNumIndices; j++)
					indices.push_back(face.mIndices[j]);
			}

			//process material
			if (mesh->mMaterialIndex >= 0)
			{
				//retrieve ai Material object from the scene's mMaterials array
				//then load mesh's diffuse and/or specular textures
				aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
				// we assume a convention for sampler names in the shaders. 
				// Each diffuse texture should be named as 'texture_diffuseN' 
				// where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
				// Same applies to other texture as the following list summarizes:
				// diffuse: texture_diffuseN
				// specular: texture_specularN
				// normal: texture_normalN
			
				// 1. diffuse maps 
				vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
				textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
				// 2. specular maps
				vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
				textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
				// 3. normal maps
				vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
				textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
				// 4. height maps
				vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
				textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
				//store Texture Structs at the end of the model's textures vector
			}

			// return a mesh object created from the extracted mesh data
			return Mesh(vertices, indices, textures);
		};

		// checks all material textures of a given type and loads the textures if they're 
		// not loaded yet. the required info is returned as a Texture struct.
		vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
		{
			
			vector<Texture> textures;
			//iterates over all the texture locations of the given texture type, 
			for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
			{
				aiString str;
				//GetTextureCount => check amount of textures stored int the material
				mat->GetTexture(type, i, &str);
				// check if texture was loaded before and if so, continue to next iteration: 
				// skip loading a new texture
				bool skip = false;
				//check for doubles 
				for (unsigned int j = 0; j < textures_loaded.size(); j++) {
					if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
					{
						textures.push_back(textures_loaded[j]);
						skip = true;
						// a texture with the same filepath has already been loaded, 
						// continue to next one. (optimization)
						break;
					}
				}

				if (!skip) {
					// if texture hasn't been loaded already, load it

					//stores the result (texture's file locations) in an aiString,
					//retrieves the texture's location and then loads and generates the texture 
					Texture texture;
					texture.id = TextureFromFile(str.C_Str(), this->directory, this->gammaCorrection);
					//loads a texture (with SOIL), returns the texture's ID
					texture.type = typeName;
					texture.path = str.C_Str();
					textures.push_back(texture); // add to loaded textures
					//stores the information
					textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
				}

				
			}
			return textures;
		};

		unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
		{
			string filename = string(path);
			filename = directory + '/' + filename;

			unsigned int textureID;
			glGenTextures(1, &textureID);

			int width, height, nrComponents;
			unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
			if (data)
			{
				GLenum format;
				if (nrComponents == 1)
					format = GL_RED;
				else if (nrComponents == 3)
					format = GL_RGB;
				else if (nrComponents == 4)
					format = GL_RGBA;

				glBindTexture(GL_TEXTURE_2D, textureID);
				glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
				glGenerateMipmap(GL_TEXTURE_2D);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				stbi_image_free(data);
			}
			else
			{
				std::cout << "Texture failed to load at path: " << filename << std::endl;
				stbi_image_free(data);
			}

			return textureID;
		};

};

#endif // ! MODEL_H
