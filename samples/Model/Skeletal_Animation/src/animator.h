#pragma once

#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "animation.h"
#include "bone.h"

/// <summary>
/// reads the hierarchy of AssimpNodeData, Interpolate all bones in a recursive manner &
/// then prepare final bone transformation matrices
/// </summary>
class Animator
{
public:
	/// <summary>
	/// Initializes a new instance of the <see cref="Animator"/> class.
	/// </summary>
	/// <param name="animation">The animation ot play</param>
	Animator(Animation* animation)
	{
		currentTime = 0.0;
		currentAnimation = animation;

		// reserve memory for final bone matrices
		finalBoneMatrices.reserve(100);

		// init bone matrices with 1.0
		for (int i = 0; i < 100; i++)
			finalBoneMatrices.push_back(glm::mat4(1.0f));
	}

	void UpdateAnimation(float dt)
	{
		deltaTime = dt;
		// if animation is existing
		if (currentAnimation)
		{
			// advances the current time with the rate of the ticksPerSecond
			currentTime += currentAnimation->GetTicksPerSecond() * dt;
			currentTime = fmod(currentTime, currentAnimation->GetDuration());
			// calculates bone transform for current time
			// start with identity matrix glm::mat4(1.0f) & root node of currentAnimation
			CalculateBoneTransform(&currentAnimation->GetRootNode(), glm::mat4(1.0f));
		}
	}

	void PlayAnimation(Animation* pAnimation)
	{
		currentAnimation = pAnimation;
		currentTime = 0.0f;	// reset current time
	}

	void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
	{
		std::string nodeName = node->name;
		glm::mat4 nodeTransform = node->transformation;

		// find bone name in current animation
		Bone* Bone = currentAnimation->FindBone(nodeName);

		if (Bone)
		{
			// interpolates all bone transforms
			Bone->Update(currentTime);
			// query bone's local transform
			nodeTransform = Bone->GetLocalTransform();
		}

		// multiply bone with parent transform to bring from local space to global space
		glm::mat4 globalTransformation = parentTransform * nodeTransform;

		auto boneInfoMap = currentAnimation->GetBoneIDMap();
		if (boneInfoMap.find(nodeName) != boneInfoMap.end())
		{
			int index = boneInfoMap[nodeName].id;
			glm::mat4 offset = boneInfoMap[nodeName].offset;
			finalBoneMatrices[index] = globalTransformation * offset;
		}

		// recursively loop over children bones
		for (int i = 0; i < node->childrenCount; i++) {
			CalculateBoneTransform(&node->children[i], globalTransformation);
		}
	}

	std::vector<glm::mat4> GetFinalBoneMatrices()
	{
		return finalBoneMatrices;
	}

private:
	std::vector<glm::mat4> finalBoneMatrices;
	Animation* currentAnimation;
	float currentTime;
	float deltaTime;

};
