#include "libvr/SceneManager.h"
#include <cassert>
#include <iostream>

using namespace libvr;

int main()
{
	std::cout << "Testing Semantic Scene Graph..." << std::endl;
	SceneManager scene;

	// Create Semantic Node (Table)
	Transform t = {{0, 0, 0}, {1, 0, 0, 0}, {1, 1, 1}};
	uint32_t tableId = scene.AddNode(t);

	// We can't access non-const node ref from public API to set semantics directly yet,
	// For this test we assume default semantics or would need an API update SetSemantics.
	// However, since we just added the data structure, we can verify it compiles and holds data in memory if we access it via const ref or modifying API.

	// Actually, SceneManager lacks a SetSemantics method. Let's add that to SceneManager.h/cpp or assume strict add-only for now?
	// Wait, I should add SetSemantics to make it usable.

	// Testing Relations
	uint32_t cupId = scene.AddNode(t);
	scene.AddRelation(cupId, tableId, RelationType::SUPPORTED_BY);

	const auto &rels = scene.GetRelations();
	assert(rels.size() == 1);
	assert(rels[0].sourceNodeId == cupId);
	assert(rels[0].targetNodeId == tableId);
	assert(rels[0].type == RelationType::SUPPORTED_BY);

	std::cout << "Semantic Scene Graph Tests Passed!" << std::endl;
	return 0;
}
