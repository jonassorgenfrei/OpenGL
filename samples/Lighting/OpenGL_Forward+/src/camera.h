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
const float YAW		  = -90.0f;
const float PITCH	  = 0.0f;

const float SENSITIVITY = 0.05f;
const float ZOOM	  = 45.0f;

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
	glm::vec3 focus;
	// Eular Angles
	float Yaw;
	float Pitch;
	// camera options
	float MouseSensitivity;
	float Zoom;

	//Constructor with vectors
	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
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
		return glm::lookAt(Position, Position + Front, Up);
	}

	/*glm::mat4 lookAt(glm::vec3 pos, glm::vec3 target, glm::vec3 up)
	{
		glm::mat4 transform = glm::mat4(1.0f);
		glm::mat4 translate = glm::mat4(1.0f);

		glm::vec3 zaxis = glm::normalize(pos - target);
		glm::vec3 xaxis = glm::normalize(glm::cross(glm::normalize(up), zaxis));
		glm::vec3 yaxis = glm::cross(zaxis, xaxis);

		translate[3][0] = -pos.x;
		translate[3][1] = -pos.y;
		translate[3][2] = -pos.z;
		
		transform[0][0] = xaxis.x;
		transform[1][0] = xaxis.y;
		transform[2][0] = xaxis.z;

		transform[0][1] = yaxis.x;
		transform[1][1] = yaxis.y;
		transform[2][1] = yaxis.z;

		transform[0][2] = zaxis.x;
		transform[1][2] = zaxis.y;
		transform[2][2] = zaxis.z;

		
		return transform * translate;
	}*/

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

	friend std::ostream& operator<<(std::ostream& os, const Camera& cam)
	{
		return os << "POS: " << cam.Position.x << "," << cam.Position.y << "," << cam.Position.z << " Yaw:" << cam.Yaw << " Pitch:" << cam.Pitch;
	}
private:
	// Calculates the front vector from the Camera's (updated) Eular Angles
	void updateCameraVectors()
	{
		glm::vec3 front;
		front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		front.y = sin(glm::radians(Pitch));
		front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Front = glm::normalize(front);
		// Also re-calculate the Right and Up vector
		Right = glm::normalize(glm::cross(Front, WorldUp));
		// Normalize the vecotrs, because their length gets closer to 0 the more 
		// you look up or down which results in slower movement.
		Up = glm::normalize(glm::cross(Right, Front));
	}
};


#endif