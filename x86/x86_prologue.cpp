#include "x86_prologue.h"

X86Prologue::X86Prologue(RDContext* ctx): m_context(ctx)
{
    m_document = RDContext_GetDocument(ctx);
    m_loader = RDContext_GetLoader(ctx);
}

void X86Prologue::search()
{
    RDDocument_EachSegment(m_document, [](const RDSegment* s, void* userdata) {
        if(!HAS_FLAG(s, SegmentFlags_Code)) return true;
        auto* thethis = reinterpret_cast<X86Prologue*>(userdata);
        const RDBlockContainer* blocks = RDDocument_GetBlocks(thethis->m_document, s->address);
        if(blocks) thethis->searchPrologue(blocks);
        return true;
    }, this);
}

std::string X86Prologue::getPrologue() const
{
    if(RDContext_MatchAssembler(m_context, "x86_64"))
        return "";

    return "55 8bec 83ec??"; // x86
}

void X86Prologue::searchPrologue(const RDBlockContainer* blocks)
{
    m_pattern = this->getPrologue();

    if(m_pattern.empty())
    {
        rd_log("Skipping prologue analysis...");
        return;
    }

    RDBlockContainer_Each(blocks, [](const RDBlock* b, void* userdata) {
        if(!IS_TYPE(b, BlockType_Unknown)) return true;
        auto* thethis = reinterpret_cast<X86Prologue*>(userdata);

        RDBufferView view;
        if(!RDContext_GetBlockView(thethis->m_context, b, &view)) return true;
        rd_status("Searching prologues @ " + rd_tohex(b->address));

        while(u8* p = RDBufferView_FindPatternNext(&view, thethis->m_pattern.c_str())) {
            auto loc = RD_AddressOf(thethis->m_loader, p);
            if(!loc.valid) continue;
            rd_log("Found prologue @ " + rd_tohex(loc.address));
            thethis->m_prologues.insert(loc.address);
        }

        return true;
    }, this);

    for(rd_address address : m_prologues)
        RDContext_DisassembleFunction(m_context, address, nullptr);
}
