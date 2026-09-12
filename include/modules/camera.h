#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <cmath>

enum Camera_Movement
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

constexpr float YAW = -90.0f;
constexpr float PITCH = 0.0f;
constexpr float SPEED = 2.5f;
constexpr float SENSITIVITY = 0.1f;
constexpr float ZOOM = 45.0f;

/** The original yaw/pitch camera, retained for teaching and comparison. */
class EulerCamera
{
public:
	glm::vec3 Position;
	glm::vec3 Front{0.0f, 0.0f, -1.0f};
	glm::vec3 Up{0.0f, 1.0f, 0.0f};
	glm::vec3 Right{1.0f, 0.0f, 0.0f};
	glm::vec3 WorldUp;
	glm::vec3 focus{0.0f, 0.0f, -1.0f};
	float Yaw;
	float Pitch;
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	EulerCamera(glm::vec3 position = glm::vec3(0.0f),
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
		float yaw = YAW, float pitch = PITCH)
		: Position(position), WorldUp(up), Yaw(yaw), Pitch(pitch),
		  MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		updateCameraVectors();
	}

	EulerCamera(float posX, float posY, float posZ,
		float upX, float upY, float upZ, float yaw, float pitch)
		: EulerCamera(glm::vec3(posX, posY, posZ), glm::vec3(upX, upY, upZ), yaw, pitch)
	{
	}

	glm::mat4 GetViewMatrix() const
	{
		return glm::lookAt(Position, Position + Front, Up);
	}

	void translate(Camera_Movement direction, float velocity) { move(direction, velocity); }

	void ProcessKeyboard(Camera_Movement direction, float deltaTime)
	{
		move(direction, MovementSpeed * deltaTime);
	}

	void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
	{
		rotate(xoffset, yoffset, constrainPitch);
	}

	void rotate(float xoffset, float yoffset, GLboolean constrainPitch = true)
	{
		Yaw += xoffset * MouseSensitivity;
		Pitch += yoffset * MouseSensitivity;
		if (constrainPitch)
			Pitch = std::clamp(Pitch, -89.0f, 89.0f);
		updateCameraVectors();
	}

	void ProcessMouseScroll(float yoffset)
	{
		Zoom = std::clamp(Zoom - yoffset, 1.0f, 45.0f);
	}

private:
	void move(Camera_Movement direction, float velocity)
	{
		if (direction == FORWARD) Position += Front * velocity;
		if (direction == BACKWARD) Position -= Front * velocity;
		if (direction == LEFT) Position -= Right * velocity;
		if (direction == RIGHT) Position += Right * velocity;
		if (direction == UP) Position += Up * velocity;
		if (direction == DOWN) Position -= Up * velocity;
	}

	void updateCameraVectors()
	{
		glm::vec3 front;
		front.x = std::cos(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));
		front.y = std::sin(glm::radians(Pitch));
		front.z = std::sin(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));
		Front = glm::normalize(front);
		Right = glm::normalize(glm::cross(Front, WorldUp));
		Up = glm::normalize(glm::cross(Right, Front));
		focus = Front;
	}
};

/**
 * Quaternion-backed camera. Rotations are accumulated as normalized
 * quaternion deltas; Yaw and Pitch remain available for compatibility and
 * diagnostics in the existing samples.
 */
class QuaternionCamera
{
public:
	glm::vec3 Position;
	glm::vec3 Front{0.0f, 0.0f, -1.0f};
	glm::vec3 Up{0.0f, 1.0f, 0.0f};
	glm::vec3 Right{1.0f, 0.0f, 0.0f};
	glm::vec3 WorldUp;
	glm::vec3 focus{0.0f, 0.0f, -1.0f};
	float Yaw;
	float Pitch;
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	QuaternionCamera(glm::vec3 position = glm::vec3(0.0f),
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
		float yaw = YAW, float pitch = PITCH)
		: Position(position), WorldUp(glm::normalize(up)), Yaw(yaw), Pitch(pitch),
		  MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		setOrientationFromAngles();
	}

	QuaternionCamera(float posX, float posY, float posZ,
		float upX, float upY, float upZ, float yaw, float pitch)
		: QuaternionCamera(glm::vec3(posX, posY, posZ), glm::vec3(upX, upY, upZ), yaw, pitch)
	{
	}

	glm::mat4 GetViewMatrix() const
	{
		const glm::mat4 rotation = glm::mat4_cast(glm::conjugate(orientation_));
		const glm::mat4 translation = glm::translate(glm::mat4(1.0f), -Position);
		return rotation * translation;
	}

	const glm::quat& Orientation() const { return orientation_; }

	void translate(Camera_Movement direction, float velocity) { move(direction, velocity); }

	void ProcessKeyboard(Camera_Movement direction, float deltaTime)
	{
		move(direction, MovementSpeed * deltaTime);
	}

	void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
	{
		rotate(xoffset, yoffset, constrainPitch);
	}

	void rotate(float xoffset, float yoffset, GLboolean constrainPitch = true)
	{
		const float yawDelta = xoffset * MouseSensitivity;
		float pitchDelta = yoffset * MouseSensitivity;
		if (constrainPitch)
		{
			const float constrainedPitch = std::clamp(Pitch + pitchDelta, -89.0f, 89.0f);
			pitchDelta = constrainedPitch - Pitch;
			Pitch = constrainedPitch;
		}
		else
		{
			Pitch += pitchDelta;
		}
		Yaw += yawDelta;

		const glm::quat yawRotation = glm::angleAxis(glm::radians(-yawDelta), WorldUp);
		orientation_ = glm::normalize(yawRotation * orientation_);
		updateCameraVectors();
		const glm::quat pitchRotation = glm::angleAxis(glm::radians(pitchDelta), Right);
		orientation_ = glm::normalize(pitchRotation * orientation_);
		updateCameraVectors();
	}

	void ProcessMouseScroll(float yoffset)
	{
		Zoom = std::clamp(Zoom - yoffset, 1.0f, 45.0f);
	}

private:
	glm::quat orientation_{1.0f, 0.0f, 0.0f, 0.0f};

	void move(Camera_Movement direction, float velocity)
	{
		if (direction == FORWARD) Position += Front * velocity;
		if (direction == BACKWARD) Position -= Front * velocity;
		if (direction == LEFT) Position -= Right * velocity;
		if (direction == RIGHT) Position += Right * velocity;
		if (direction == UP) Position += Up * velocity;
		if (direction == DOWN) Position -= Up * velocity;
	}

	void setOrientationFromAngles()
	{
		const glm::quat yawRotation = glm::angleAxis(glm::radians(-(Yaw + 90.0f)), WorldUp);
		const glm::vec3 initialRight = yawRotation * glm::vec3(1.0f, 0.0f, 0.0f);
		const glm::quat pitchRotation = glm::angleAxis(glm::radians(Pitch), initialRight);
		orientation_ = glm::normalize(pitchRotation * yawRotation);
		updateCameraVectors();
	}

	void updateCameraVectors()
	{
		Front = glm::normalize(orientation_ * glm::vec3(0.0f, 0.0f, -1.0f));
		Right = glm::normalize(orientation_ * glm::vec3(1.0f, 0.0f, 0.0f));
		Up = glm::normalize(orientation_ * glm::vec3(0.0f, 1.0f, 0.0f));
		focus = Front;
	}
};

// Regular samples use quaternion orientation without call-site changes.
using Camera = QuaternionCamera;

#endif
