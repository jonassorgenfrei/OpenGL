#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include <glad\glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

/**
* Defines several possible options for camera movement.
* Used as abstraction to stay away from window-system
* specific input methods
*/
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float ROLL = 0.0f;
const float SENSITIVITY = 0.05f;
const float ZOOM = 45.0f;

// An abstract camera class that process input and calculates the corresponding 
// Eular Angles, Vectors and Matrices for use in OpenGl
class Camera
{
public:
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;
	//Focus Point
	float distance = 1.0f;

	float radius = 1.0f;
	// Eular Angles
	float Yaw;
	float Pitch;
	float Roll;
	// camera options
	float MouseSensitivity;
	float Zoom;

	//Constructor with vectors
	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH, float roll = ROLL) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		Position = position;
		WorldUp = up;
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}

	// constructor with scalar values
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		Position = glm::vec3(posX, posY, posZ);
		WorldUp = glm::vec3(upX, upY, upZ);
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}

	// Returns the view matrix calculated using Eular Angles and the LookAt Matrix
	glm::mat4 GetViewMatrix()
	{
		//FPS camera:  RotationX(pitch) * RotationY(yaw)
		glm::quat qPitch = glm::angleAxis(Pitch, glm::vec3(1, 0, 0));
		glm::quat qYaw = glm::angleAxis(Yaw, glm::vec3(0, 1, 0));
		glm::quat qRoll = glm::angleAxis(Roll, glm::vec3(0, 0, 1));

		//For a FPS camera we can omit roll
		glm::quat orientation = qPitch * qYaw;
		orientation = glm::normalize(orientation);
		glm::mat4 rotate = glm::mat4_cast(orientation);

		glm::mat4 translate = glm::mat4(1.0f);
		translate = glm::translate(translate, -Position);

		return rotate * translate;

	}


	// Process input received from any keyboard-like input system. 
	// Accepts input parameter in the form of camera defined ENUM
	// (to abstract it from windowing systems)
	void translate(Camera_Movement direction, float velocity)
	{
		if (direction == FORWARD)
			Position += Front * velocity;
		if (direction == BACKWARD)
			Position -= Front * velocity;
		if (direction == LEFT)
			Position -= Right * velocity;
		if (direction == RIGHT)
			Position += Right * velocity;
		if (direction == UP)
			Position += Up * velocity;
		if (direction == DOWN)
			Position -= Up * velocity;

		updateCameraVectors();
	}

	// processes input received from a mouse input system.
	// Expects the offset value in both the x and y direction.
	void rotate(float xoffset, float yoffset, GLboolean constrainPitch = true)
	{

		/* TODO: ARC-BALL CAM */

		xoffset *= MouseSensitivity;
		yoffset *= MouseSensitivity;

		Yaw += xoffset;
		Pitch += yoffset;

		// Make sure that when pitch is out of bounds, screen doen't get flipped
		if (constrainPitch)
		{
			if (Pitch > 89.0f)
				Pitch = 89.0f;
			if (Pitch < -89.0f)
				Pitch = -89.0f;
		}

		//Update Front, Right and Up Vectors using the updated Eular angles
		updateCameraVectors();
	}

	//Processess input received from a mouse scroll-wheel event.
	// Only requires input on the vertical wheel-axis
	void ProcessMouseScroll(float yoffset)
	{
		if (Zoom >= 1.0f && Zoom <= 45.0f)
			Zoom -= yoffset;
		if (Zoom <= 1.0f)
			Zoom = 1.0f;
		if (Zoom >= 45.0f)
			Zoom = 45.0f;
	}

private:
	// Calculates the front vector from the Camera's (updated) Eular Angles
	void updateCameraVectors()
	{
		/*glm::vec3 front;*/
		Position.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Position.y = sin(glm::radians(Pitch));
		Position.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

		//Front = glm::normalize(focus - Position);
		// Also re-calculate the Right and Up vector
		Right = glm::normalize(glm::cross(Front, WorldUp));
		// Normalize the vecotrs, because their length gets closer to 0 the more 
		// you look up or down which results in slower movement.
		Up = glm::normalize(glm::cross(Right, Front));
	}
};
#endif