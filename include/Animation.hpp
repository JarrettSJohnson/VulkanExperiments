#pragma once

#include <chrono>
#include <variant>

struct TransformComponents {
  glm::vec3 pos;
  float pitch;
  float yaw;
  float roll;
  glm::vec3 scale;
  TransformComponents operator-(const TransformComponents& other) const
  {
    TransformComponents comps;
    comps.pos = pos - other.pos;
    comps.pitch = pitch - other.pitch;
    comps.yaw = yaw - other.yaw;
    comps.roll = roll - other.roll;
    comps.scale = scale - other.scale;
    return comps;
  }
  std::string to_string() const
  {
    std::string ret = std::string("Transform info:\n") +
                      "pos: " + printVec3(pos) + '\n' +
                      "pitch: " + std::to_string(pitch) + '\n' +
                      "yaw: " + std::to_string(yaw) + '\n' +
                      "roll: " + std::to_string(roll) + '\n' +
                      "scale: " + printVec3(scale) + '\n';
    return ret;
  }
};

class Linear
{
public:
  Linear() = default;
  Linear(TransformComponents start, TransformComponents end)
      : start(start), end(end)
  {
  }
  TransformComponents animate(float reldt)
  {
    TransformComponents comps{};
    comps.pos = reldt * end.pos - reldt * start.pos;
    comps.pitch = reldt * end.pitch - reldt * start.pitch;
    comps.yaw = reldt * end.yaw - reldt * start.yaw;
    comps.roll = reldt * end.roll - reldt * start.roll;
    comps.scale = reldt * end.scale - reldt * start.scale;
    return comps;
  }
  private:
  TransformComponents start{};
  TransformComponents end{};
};

struct FrameAnimInfo {
  float leftover;
  TransformComponents comps;
};

class KeyFrame
{
private:
  std::variant<Linear> m_interpMode{};
  float m_progress{};
  float m_duration{};
  TransformComponents lastComponents{};

public:
  KeyFrame(float duration) : m_duration{duration} {}
  template <typename T, typename... Args>
  void setTranslationInterpMode(Args&&... args)
  {
    m_interpMode.emplace<T>(std::forward<Args>(args)...);
  }
  FrameAnimInfo animate(float dt)
  {
    auto reldt = std::min(1.0f, (m_progress + dt) / m_duration);
    
    auto travel = std::visit(
        [reldt](auto& inter) { return inter.animate(reldt); }, m_interpMode);

    auto trans = travel - lastComponents;
    lastComponents = travel;

    auto leftover = m_progress + dt - m_duration;
    m_progress += dt;
    m_progress = std::clamp(m_progress, 0.0f, m_duration);
    return FrameAnimInfo{std::max(0.0f, leftover), trans};
  }
  void reset()
  {
    m_progress = 0;
    lastComponents = TransformComponents{};
  }
};

class Animation
{
private:
  using KeyFrameIdx = std::size_t;
  std::vector<KeyFrame> keyframes;
  TransformComponents* m_tcomponents{nullptr};
  KeyFrameIdx m_idx{};
  bool m_loop{false};
  bool m_animating{false};

public:
  Animation(bool loop) : m_loop{loop} {}
  void setTransform(TransformComponents& transform)
  {
    m_tcomponents = &transform;
    startingPos = transform;
  }
  TransformComponents* getTransform() const { return m_tcomponents; }
  TransformComponents startingPos{};
  KeyFrame& addKeyFrame(float duration)
  {
    return keyframes.emplace_back(duration);
  }
  bool animate(float dt)
  {
    if (!m_animating && m_tcomponents && !keyframes.empty()) {
      *m_tcomponents = startingPos;
      m_animating = true;
	}
    for (; m_idx < keyframes.size(); ++m_idx) {
      auto [leftover, transComp] = keyframes[m_idx].animate(dt);

      m_tcomponents->pos += transComp.pos;
      m_tcomponents->yaw += transComp.yaw;
      m_tcomponents->pitch += transComp.pitch;
      dt = leftover;
      if (dt < 1e-6) {
        break;
      }
    }
    if (m_idx == keyframes.size() && m_loop) {
      for (auto& kf : keyframes) {
        m_animating = false;
        kf.reset();
      }
      m_idx = 0;
    }
    return true;
  }
};

class AnimateSystem
{
private:
  std::vector<Animation> m_animations;

public:
  AnimateSystem() = default;
  void bind(std::size_t animIdx, TransformComponents& transform)
  {
    m_animations[animIdx].setTransform(transform);
  }

  bool animate(float dt)
  {
    return m_animations.front().animate(dt);
    for (auto& animation : m_animations) {
      if (animation.animate(dt)) {
      }
    }
  }
  Animation& addAnimation(bool loop = false)
  {
    return m_animations.emplace_back(loop);
  }
  void addAnimations(const std::vector<bool>& vec)
  {
    for (auto v : vec) {
      addAnimation(v);
    }
  }
};
