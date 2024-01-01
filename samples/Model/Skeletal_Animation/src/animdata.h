#pragma once

#include<glm/glm.hpp>

/// <summary>
/// Stores the offset matrix and a unique id which will be used as an index to store it in 
/// finalBoneMatrices array 
/// </summary>
struct BoneInfo
{
	/*id is index in finalBoneMatrices*/
	int id;

	/*offset matrix transforms vertex from model space to bone space*/
	glm::mat4 offset;

};
#pragma once
