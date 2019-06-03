#pragma once


inline std::string printVec3(const glm::vec3 vec)
{
  std::string str = "(";
  str += std::to_string(vec.x);
  str += ", ";
  str += std::to_string(vec.y);
  str += ", ";
  str += std::to_string(vec.z);
  str += ")";
  return str;
}