#pragma once

/* Container for bone data */

#include <vector>
#include <assimp/scene.h>
#include <list>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include "assimp_glm_helpers.h"

// structs for each keyframe type 
// translation, rotation & scale

struct KeyPosition
{
	glm::vec3 position;
	float timeStamp; // tells at what point of an animation the value needs to be used for interpolation
};

struct KeyRotation
{
	glm::quat orientation;
	float timeStamp;
};

struct KeyScale
{
	glm::vec3 scale;
	float timeStamp;
};

/// <summary>
/// single bone which reads all keyframedata from aiNodeAnim
/// It will also interpolate between its keys i.e. translation,
/// scale & rotation (based on current anim time)
/// </summary>
class Bone
{
public:
	/// <summary>
	/// Initializes a new instance of the <see cref="Bone"/> class.
	/// Reads from aiNodeAnim and stores keys and their timestamps to member variables.
	/// </summary>
	/// <param name="name">The name.</param>
	/// <param name="ID">The identifier.</param>
	/// <param name="channel">Assimp aiNodAnim</param>
	Bone(const std::string& name, int ID, const aiNodeAnim* channel)
		:
		name(name),
		ID(ID),
		localTransform(1.0f)
	{
		// read & store position
		numPositions = channel->mNumPositionKeys;

		for (int positionIndex = 0; positionIndex < numPositions; ++positionIndex)
		{
			aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
			float timeStamp = channel->mPositionKeys[positionIndex].mTime;
			KeyPosition data;
			data.position = AssimpGLMHelpers::GetGLMVec(aiPosition);
			data.timeStamp = timeStamp;
			positions.push_back(data);	
		}

		// read & store rotation
		numRotations = channel->mNumRotationKeys;
		for (int rotationIndex = 0; rotationIndex < numRotations; ++rotationIndex)
		{
			aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
			float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
			KeyRotation data;
			data.orientation = AssimpGLMHelpers::GetGLMQuat(aiOrientation);
			data.timeStamp = timeStamp;
			rotations.push_back(data);
		}

		// read & store scaling
		numScalings = channel->mNumScalingKeys;
		for (int keyIndex = 0; keyIndex < numScalings; ++keyIndex)
		{
			aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
			float timeStamp = channel->mScalingKeys[keyIndex].mTime;
			KeyScale data;
			data.scale = AssimpGLMHelpers::GetGLMVec(scale);
			data.timeStamp = timeStamp;
			scales.push_back(data);
		}
	}
	
	/// <summary>
	/// Main Interpolation process
	/// Get's called every frame
	/// </summary>
	/// <param name="animationTime">The animation time.</param>
	void Update(float animationTime)
	{
		// call of Interpolation for each Transformation
		glm::mat4 translation = InterpolatePosition(animationTime);
		glm::mat4 rotation = InterpolateRotation(animationTime);
		glm::mat4 scale = InterpolateScaling(animationTime);
		// Combine final Interpolation Transformations to a single localTransform matrix
		// NOTE: TRS-Transform 
		localTransform = translation * rotation * scale;
	}

	glm::mat4 GetLocalTransform() { return localTransform; }

	std::string GetBoneName() const { return name; }

	int GetBoneID() { return ID; }

	int GetPositionIndex(float animationTime)
	{
		for (int index = 0; index < numPositions - 1; ++index)
		{
			if (animationTime < positions[index + 1].timeStamp)
				return index;
		}
		assert(0);
	}

	int GetRotationIndex(float animationTime)
	{
		for (int index = 0; index < numRotations - 1; ++index)
		{
			if (animationTime < rotations[index + 1].timeStamp)
				return index;
		}
		assert(0);
	}

	int GetScaleIndex(float animationTime)
	{
		for (int index = 0; index < numScalings - 1; ++index)
		{
			if (animationTime < scales[index + 1].timeStamp)
				return index;
		}
		assert(0);
	}


private:

	/// <summary>
	/// Calculates ratio between timestamp of p0 and p1 based on anim time
	/// </summary>
	/// <param name="lastTimeStamp">The last time stamp.</param>
	/// <param name="nextTimeStamp">The next time stamp.</param>
	/// <param name="animationTime">The animation time.</param>
	/// <returns>Ratio between timestamps for interpolation in range [0,1]</returns>
	float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
	{
		float scaleFactor = 0.0f;
		float midWayLength = animationTime - lastTimeStamp;
		float framesDiff = nextTimeStamp - lastTimeStamp;
		scaleFactor = midWayLength / framesDiff;
		return scaleFactor;
	}

	/* Interpolation functions */

	glm::mat4 InterpolatePosition(float animationTime)
	{
		if (1 == numPositions)
			return glm::translate(glm::mat4(1.0f), positions[0].position);

		int p0Index = GetPositionIndex(animationTime);
		int p1Index = p0Index + 1;

		float scaleFactor = GetScaleFactor(positions[p0Index].timeStamp, positions[p1Index].timeStamp, animationTime);
		// using mix/linear interpolation
		glm::vec3 finalPosition = glm::mix(positions[p0Index].position, positions[p1Index].position, scaleFactor);
		return glm::translate(glm::mat4(1.0f), finalPosition);
	}

	glm::mat4 InterpolateRotation(float animationTime)
	{
		if (1 == numRotations)
		{
			auto rotation = glm::normalize(rotations[0].orientation);
			return glm::toMat4(rotation);
		}

		int p0Index = GetRotationIndex(animationTime);
		int p1Index = p0Index + 1;

		float scaleFactor = GetScaleFactor(rotations[p0Index].timeStamp, rotations[p1Index].timeStamp, animationTime);
		// using spherical linear interpolation for rotation interpolation of quaternions
		// first argument	- last key
		// second argument	- next key
		// third argument	- interpolation in range [0,1]
		glm::quat finalRotation = glm::slerp(rotations[p0Index].orientation, rotations[p1Index].orientation, scaleFactor);
		finalRotation = glm::normalize(finalRotation);
		return glm::toMat4(finalRotation);

	}

	glm::mat4 InterpolateScaling(float animationTime)
	{
		if (1 == numScalings)
			return glm::scale(glm::mat4(1.0f), scales[0].scale);

		int p0Index = GetScaleIndex(animationTime);
		int p1Index = p0Index + 1;

		float scaleFactor = GetScaleFactor(scales[p0Index].timeStamp, scales[p1Index].timeStamp, animationTime);

		// using mix/linear interpolation
		glm::vec3 finalScale = glm::mix(scales[p0Index].scale, scales[p1Index].scale, scaleFactor);
		return glm::scale(glm::mat4(1.0f), finalScale);
	}

	// keyframes list
	std::vector<KeyPosition> positions;
	std::vector<KeyRotation> rotations;
	std::vector<KeyScale> scales;

	// length of each list
	int numPositions;
	int numRotations;
	int numScalings;

	glm::mat4 localTransform;
	std::string name;
	int ID;
};

