#pragma once

#include "Model.hpp"

/*class FanganModel
{
public:
  FanganModel(Model& model, Texture& texture, glm::vec3 focus, std::size_t
charIdx) : m_model{&model}, m_texture{&texture}, m_focus{focus},
m_charIdx{charIdx}
  {
  }
  std::size_t getIdx() const { return m_charIdx; }
  private:
  Model* m_model{};
  Texture* m_texture{};
  glm::vec3 m_focus{};
  std::size_t m_charIdx{};
};*/

struct FanganModel {
  Model* m_model;
  Texture* m_texture;
  glm::vec3 m_focus;
  std::size_t m_charIdx;
  FanganModel(
      Model& model, Texture& texture, glm::vec3 focus, std::size_t charIdx)
      : m_model{&model}, m_texture{&texture}, m_focus{focus}, m_charIdx{charIdx}
  {
  }
};