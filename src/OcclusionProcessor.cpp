#include "../include/OcclusionProcessor.h"

OcclusionProcessor::OcclusionProcessor() { RegisterDefaultMaterials(); }

void OcclusionProcessor::SetQueryCallback(OcclusionQueryCallback callback) {
  m_QueryCallback = std::move(callback);
}

void OcclusionProcessor::RegisterMaterial(const OcclusionMaterial &mat) {
  m_Materials[mat.name] = mat;
}

void OcclusionProcessor::SetOcclusionThreshold(float threshold) {
  m_OcclusionThreshold = std::clamp(threshold, 0.0f, 1.0f);
}

void OcclusionProcessor::SetSmoothingTime(float seconds) {
  m_SmoothingTime = std::max(seconds, 0.01f);
}

void OcclusionProcessor::SetUpdateRate(float hz) {
  m_UpdateRate = std::max(hz, 1.0f);
}

void OcclusionProcessor::SetEnabled(bool enabled) { m_Enabled = enabled; }

void OcclusionProcessor::SetLowPassRange(float minFreq, float maxFreq) {
  m_MinLowPassFreq = std::clamp(minFreq, 100.0f, 22000.0f);
  m_MaxLowPassFreq = std::clamp(maxFreq, m_MinLowPassFreq, 22000.0f);
}

void OcclusionProcessor::SetVolumeReduction(float maxReduction) {
  m_MaxVolumeReduction = std::clamp(maxReduction, 0.0f, 1.0f);
}

void OcclusionProcessor::Update(Voice &voice, const Vector3 &listenerPos,
                                float dt) {
  if (!m_Enabled || !m_QueryCallback) {
    voice.obstruction = 0.0f;
    voice.occlusion = 0.0f;
    voice.targetLowPassFreq = m_MaxLowPassFreq;
    voice.occlusionVolume = 1.0f;
    SmoothValues(voice, dt);
    return;
  }

  m_TimeSinceLastUpdate += dt;
  if (m_TimeSinceLastUpdate < 1.0f / m_UpdateRate) {
    SmoothValues(voice, dt);
    return;
  }
  m_TimeSinceLastUpdate = 0.0f;

  auto hits = m_QueryCallback(voice.position, listenerPos);

  float totalObstruction = 0.0f;
  float totalOcclusionBias = 0.0f;

  for (const auto &hit : hits) {
    const OcclusionMaterial &mat = GetMaterial(hit.materialName);
    float thicknessFactor = std::min(hit.thickness, 3.0f) / 3.0f;
    totalObstruction += mat.obstruction * (0.5f + 0.5f * thicknessFactor);
    totalOcclusionBias += mat.occlusionBias;
  }

  voice.obstruction = std::clamp(totalObstruction, 0.0f, 1.0f);

  float occlusionValue = voice.obstruction + totalOcclusionBias;
  if (occlusionValue >= m_OcclusionThreshold) {
    float t =
        (occlusionValue - m_OcclusionThreshold) / (1.0f - m_OcclusionThreshold);
    voice.occlusion = std::clamp(t, 0.0f, 1.0f);
  } else {
    voice.occlusion = 0.0f;
  }

  float combined = std::max(voice.obstruction, voice.occlusion);

  float freqT = 1.0f - combined;
  voice.targetLowPassFreq =
      m_MinLowPassFreq * std::pow(m_MaxLowPassFreq / m_MinLowPassFreq, freqT);

  voice.occlusionVolume = 1.0f - (combined * m_MaxVolumeReduction);

  SmoothValues(voice, dt);
}

void OcclusionProcessor::ApplyDSP(SoLoud::Soloud &engine, Voice &voice) {
  if (!m_Enabled || voice.handle == 0)
    return;

  float occludedVolume = voice.volume * voice.occlusionVolume;
  engine.setVolume(voice.handle, occludedVolume);

  engine.setFilterParameter(voice.handle, 0,
                            SoLoud::BiquadResonantFilter::FREQUENCY,
                            voice.currentLowPassFreq);
}

bool OcclusionProcessor::IsEnabled() const { return m_Enabled; }

float OcclusionProcessor::GetOcclusionThreshold() const {
  return m_OcclusionThreshold;
}

void OcclusionProcessor::RegisterDefaultMaterials() {
  RegisterMaterial(OcclusionMaterials::Glass);
  RegisterMaterial(OcclusionMaterials::Fabric);
  RegisterMaterial(OcclusionMaterials::Foliage);
  RegisterMaterial(OcclusionMaterials::Wood);
  RegisterMaterial(OcclusionMaterials::Plaster);
  RegisterMaterial(OcclusionMaterials::Metal);
  RegisterMaterial(OcclusionMaterials::Brick);
  RegisterMaterial(OcclusionMaterials::Concrete);
  RegisterMaterial(OcclusionMaterials::Stone);
  RegisterMaterial(OcclusionMaterials::Terrain);
  RegisterMaterial(OcclusionMaterials::Water);
  RegisterMaterial(OcclusionMaterials::Default);
}

const OcclusionMaterial &
OcclusionProcessor::GetMaterial(const std::string &name) const {
  auto it = m_Materials.find(name);
  if (it != m_Materials.end()) {
    return it->second;
  }
  static const OcclusionMaterial defaultMat = OcclusionMaterials::Default;
  return defaultMat;
}

void OcclusionProcessor::SmoothValues(Voice &voice, float dt) {
  float alpha = 1.0f - std::exp(-dt / m_SmoothingTime);
  voice.currentLowPassFreq +=
      alpha * (voice.targetLowPassFreq - voice.currentLowPassFreq);
  voice.occlusionSmoothed +=
      alpha * (voice.occlusion - voice.occlusionSmoothed);
}
