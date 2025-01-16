#pragma once

#include <obs.hpp>

#include <vector>

enum class MultiviewLayout : uint8_t {
	HORIZONTAL_TOP_8_SCENES = 0,
	HORIZONTAL_BOTTOM_8_SCENES = 1,
	VERTICAL_LEFT_8_SCENES = 2,
	VERTICAL_RIGHT_8_SCENES = 3,
	HORIZONTAL_TOP_24_SCENES = 4,
	HORIZONTAL_TOP_18_SCENES = 5,
	SCENES_ONLY_4_SCENES = 6,
	SCENES_ONLY_9_SCENES = 7,
	SCENES_ONLY_16_SCENES = 8,
	SCENES_ONLY_25_SCENES = 9,
};

class Multiview {
public:
	Multiview();
	~Multiview();
	void Update(MultiviewLayout multiviewLayout, bool drawLabel, bool drawSafeArea);
	void Render(uint32_t cx, uint32_t cy);
	OBSSource GetSourceByPosition(int x, int y);

private:
	bool drawLabel, drawSafeArea;
	MultiviewLayout multiviewLayout;
	size_t maxSrcs, numSrcs;
	gs_vertbuffer_t *actionSafeMargin = nullptr;
	gs_vertbuffer_t *graphicsSafeMargin = nullptr;
	gs_vertbuffer_t *fourByThreeSafeMargin = nullptr;
	gs_vertbuffer_t *leftLine = nullptr;
	gs_vertbuffer_t *topLine = nullptr;
	gs_vertbuffer_t *rightLine = nullptr;

	std::vector<OBSWeakSource> multiviewScenes;
	std::vector<OBSSource> multiviewLabels;

	// Multiview position helpers
	float thickness = 6;
	float offset, thicknessx2 = thickness * 2, pvwprgCX, pvwprgCY, sourceX, sourceY, labelX, labelY, scenesCX,
		      scenesCY, ppiCX, ppiCY, siX, siY, siCX, siCY, ppiScaleX, ppiScaleY, siScaleX, siScaleY, fw, fh,
		      ratio;

	// argb colors
	static const uint32_t outerColor = 0xFF999999;
	static const uint32_t labelColor = 0x33000000;
	static const uint32_t backgroundColor = 0xFF000000;
	static const uint32_t previewColor = 0xFF00D000;
	static const uint32_t programColor = 0xFFD00000;
};

static inline void startRegion(int vX, int vY, int vCX, int vCY, float oL, float oR, float oT, float oB)
{
	gs_projection_push();
	gs_viewport_push();
	gs_set_viewport(vX, vY, vCX, vCY);
	gs_ortho(oL, oR, oT, oB, -100.0f, 100.0f);
}

static inline void endRegion()
{
	gs_viewport_pop();
	gs_projection_pop();
}
