#pragma once
#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"

#include <string>
#include <vector>
using namespace std;

struct Vertex {
	// position
	glm::vec3 Position;
	// color
	glm::vec3 Color;
	// texCoords
	glm::vec2 TexCoords;
	// normal
	glm::vec3 Normal;
	// tangent
	glm::vec3 Tangent;
	// bitangent
	glm::vec3 Bitangent;
};

struct Texture {
	unsigned int id;
	string type;
	string path;
};

class Mesh {
public:
	// mesh Data
	vector<Vertex>       vertices;
	vector<unsigned int> indices;
	vector<Texture>      textures;
	//skybox cubemap ID
	GLuint				 skyboxID;
	bool				 bCCWTris;

	unsigned int VAO;

	// constructor
	Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures, GLuint skyboxID = 0, bool	bCCWTris = true)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;

		this->skyboxID = skyboxID;

		this->bCCWTris = bCCWTris;

		// now that we have all the required data, set the vertex buffers and its attribute pointers.
		setupMesh();
	}

	// render the mesh
	void Draw(Shader &shader, GLboolean bInstanced = false, GLuint amount = 0)
	{
		shader.UseProgram();

		// bind appropriate textures
		unsigned int diffuseNr = 0;
		unsigned int specularNr = 0;
		unsigned int normalNr = 0;
		unsigned int heightNr = 0;
		unsigned int reflectionNr = 0;
		for (unsigned int i = 0; i < textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
			// retrieve texture number (the N in diffuse_textureN)
			string number;
			string name = textures[i].type;
			if (name == "diffuse")
				number = std::to_string(diffuseNr++);
			else if (name == "specular")
				number = std::to_string(specularNr++); // transfer unsigned int to stream
			else if (name == "normal")
				number = std::to_string(normalNr++); // transfer unsigned int to stream
			else if (name == "height")
				number = std::to_string(heightNr++); // transfer unsigned int to stream
			else if (name == "reflection")
				number = std::to_string(reflectionNr++); // transfer unsigned int to stream

			// now set the sampler to the correct texture unit
			string propertyName = "material." + name + "[" + number + "]";
			shader.SetInt(propertyName, i);
			//glUniform1i(glGetUniformLocation(shader.ID, ("material." + name + "[" + number + "]").c_str()), i);
			// and finally bind the texture
			glBindTexture(GL_TEXTURE_2D, textures[i].id);
		}

		if (normalNr > 0)
			shader.SetBool("bHasNormalMap", true);

		//Bind skybox cubemap
		if (this->skyboxID != 0)
		{//glEnable(GL_TEXTURE_CUBE_MAP);
			/*glActiveTexture(GL_TEXTURE0 + textures.size());
			glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);

			shader.SetInt("skybox", textures.size());*/

			glActiveTexture(GL_TEXTURE31);
			glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);

			shader.SetInt("skybox", 31);
		}
		

		//enable CW triangles order
		if(!bCCWTris)
			glFrontFace(GL_CW);
		// draw mesh
		glBindVertexArray(VAO);
		if(!bInstanced)
			glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		else
			glDrawElementsInstanced(
				GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, amount
			);
		glBindVertexArray(0);
		if (!bCCWTris)
			glFrontFace(GL_CCW);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);

		shader.SetBool("bHasNormalMap", false);
	}

private:
	// render data 
	unsigned int VBO, EBO;

	// initializes all the buffer objects/arrays
	void setupMesh()
	{
		// create buffers/arrays
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);
		// load data into vertex buffers
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		// A great thing about structs is that their memory layout is sequential for all its items.
		// The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
		// again translates to 3/2 floats which translates to a byte array.
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		// set the vertex attribute pointers
		// vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		// vertex colors
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
		// vertex normals
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
		// vertex tangent
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
		// vertex bitangent
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

		glBindVertexArray(0);
	}
};
#endif