#include "pch.h"
#include "usdiInternal.h"
#include "usdiSchema.h"
#include "usdiXform.h"
#include "usdiPolyMesh.h"
#include "usdiContext.h"

namespace usdi {



ImportContext::ImportContext()
{
}

ImportContext::~ImportContext()
{
    unload();
}

void ImportContext::unload()
{
    m_stage = UsdStageRefPtr();
    m_schemas.clear();
}


bool ImportContext::open(const char *path)
{
    unload();

    m_stage = UsdStage::Open(path);
    if (m_stage == UsdStageRefPtr()) {
        return false;
    }

    m_start_time = m_stage->GetStartTimeCode();
    m_end_time = m_stage->GetEndTimeCode();

    UsdPrim root_prim = m_prim_root.empty() ? m_stage->GetDefaultPrim() : m_stage->GetPrimAtPath(SdfPath(m_prim_root));
    if (!root_prim && !(m_prim_root.empty() || m_prim_root == "/")) {
        root_prim = m_stage->GetPseudoRoot();
    }

    // Check if prim exists.  Exit if not
    if (!root_prim.IsValid()) {
        unload();
        return false;
    }

    // Set the variants on the usdRootPrim
    for (std::map<std::string, std::string>::iterator it = m_variants.begin(); it != m_variants.end(); ++it) {
        root_prim.GetVariantSet(it->first).SetVariantSelection(it->second);
    }

    constructTreeRecursive(nullptr, root_prim);
}

Schema* ImportContext::getRootNode()
{
    return m_schemas.empty() ? nullptr : m_schemas.front().get();
}

void ImportContext::constructTreeRecursive(Schema *parent, UsdPrim prim)
{
    Schema *node = createNode(parent, prim);
    if (node) {
        m_schemas.emplace_back(node);

        auto children = prim.GetChildren();
        for (auto c : children) {
            constructTreeRecursive(node, c);
        }
    }
}

Schema* ImportContext::createNode(Schema *parent, UsdPrim prim)
{
    UsdGeomMesh mesh(prim);
    UsdGeomXform xf(prim);
    if (mesh) {
        return new Mesh(parent, mesh);
    }
    else if (xf) {
        return new Xform(parent, xf);
    }
    return nullptr;
}

} // namespace usdi