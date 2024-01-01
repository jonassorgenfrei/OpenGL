#pragma once

#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include "bone.h"
#include <functional>
#include "animdata.h"
#include "model_animation.h"

/// <summary>
/// Helper struct to isolate animation from assimp
/// </summary>
struct AssimpNodeData
{
	glm::mat4 transformation;
	std::string name;
	int childrenCount;
	std::vector<AssimpNodeData> children;
};

/// <summary>
/// Reads data from aiAnimation (assimp) and creates a hierarchical record of bones
/// </summary>
class Animation
{
public:
	Animation() = default;

	/// <summary>
	/// Initializes a new instance of the <see cref="Animation"/> class.
	/// </summary>
	/// <param name="animationPath">The filepath to the animation.</param>
	/// <param name="model">The model for this animation.</param>
	Animation(const std::string& animationPath, Model* model)
	{
		// read in animation data
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
		assert(scene && scene->mRootNode);
		auto animation = scene->mAnimations[0];
		// get duration of the animation
		duration = animation->mDuration;
		// get speed of animation
		ticksPerSecond = animation->mTicksPerSecond;
		// get global transformation matrix for bone
		aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
		globalTransformation = globalTransformation.Inverse();
		ReadHierarchyData(rootNode, scene->mRootNode);
		ReadMissingBones(animation, *model);
	}

	~Animation()
	{
	}

	Bone* FindBone(const std::string& name)
	{
		auto iter = std::find_if(bones.begin(), bones.end(),
			[&](const Bone& Bone)
			{
				return Bone.GetBoneName() == name;
			}
		);
		if (iter == bones.end()) return nullptr;
		else return &(*iter);
	}


	inline float GetTicksPerSecond() { return ticksPerSecond; }
	inline float GetDuration() { return duration; }
	inline const AssimpNodeData& GetRootNode() { return rootNode; }
	inline const std::map<std::string, BoneInfo>& GetBoneIDMap()
	{
		return boneInfoMap;
	}

private:
	void ReadMissingBones(const aiAnimation* animation, Model& model)
	{
		int size = animation->mNumChannels;

		auto& boneInfoMapModel = model.GetBoneInfoMap();//getting m_BoneInfoMap from Model class
		int& boneCount = model.GetBoneCount(); //getting the m_BoneCounter from Model class

		//reading channels(bones engaged in an animation and their keyframes)
		for (int i = 0; i < size; i++)
		{
			auto channel = animation->mChannels[i];
			std::string boneName = channel->mNodeName.data;

			if (boneInfoMapModel.find(boneName) == boneInfoMapModel.end())
			{
				boneInfoMapModel[boneName].id = boneCount;
				boneCount++;
			}
			bones.push_back(Bone(channel->mNodeName.data,
				boneInfoMapModel[channel->mNodeName.data].id, channel));
		}

		boneInfoMap = boneInfoMapModel;
	}

	/// <summary>
	/// Replicate assimp aiNode hierarchy with AssimpNodeData.
	/// </summary>
	/// <param name="dest">The dest.</param>
	/// <param name="src">The source.</param>
	void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src)
	{
		assert(src);

		dest.name = src->mName.data;
		dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
		dest.childrenCount = src->mNumChildren;

		for (int i = 0; i < src->mNumChildren; i++)
		{
			AssimpNodeData newData;
			ReadHierarchyData(newData, src->mChildren[i]);
			dest.children.push_back(newData);
		}
	}
	float duration;
	int ticksPerSecond;
	std::vector<Bone> bones;
	AssimpNodeData rootNode;
	std::map<std::string, BoneInfo> boneInfoMap;
};

