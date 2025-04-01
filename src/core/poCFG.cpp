#include "poCFG.h"
#include <algorithm>

using namespace po;

//======================
// poInstruction
//======================

poInstruction::poInstruction(const int32_t name, const int8_t type, const int16_t left, const int16_t right, const int16_t code)
    :
    _type(type),
    _left(left),
    _right(right),
    _code(code),
    _padding(0),
    _name(name)
{
}

poInstruction::poInstruction(const int32_t name, const int8_t type, const int16_t constant, const int16_t code)
    :
    _type(type),
    _constant(constant),
    _code(code),
    _padding(0),
    _right(0),
    _name(name)
{
}



//=================
// Basic Blocks
//=================

poBasicBlock::poBasicBlock()
    :
    _next(nullptr),
    _prev(nullptr),
    _branch(nullptr),
    _unconditionalBranch(false)
{
}



//=============
// Flow Graph
//=============

void poFlowGraph::addBasicBlock(poBasicBlock* bb)
{
    poBasicBlock* last = getLast();
    if (last)
    {
        last->setNext(bb);
        bb->setPrev(last);
    }
    _blocks.push_back(bb);
}

//void poFlowGraph::insertBasicBlock(int index, poBasicBlock* bb)
//{
//    _blocks.insert(_blocks.begin() + index, bb);
//}

void poFlowGraph::removeBasicBlock(poBasicBlock* bb)
{
    const auto& it = std::find(_blocks.begin(), _blocks.end(), bb);
    if (it != _blocks.end())
    {
        _blocks.erase(it);

        poBasicBlock* prev = bb->getPrev();
        poBasicBlock* next = bb->getNext();

        bb->setNext(nullptr);
        bb->setPrev(nullptr);

        if (prev && next)
        {
            prev->setNext(next);
            next->setPrev(prev);
        }
        else if (prev)
        {
            prev->setNext(nullptr);
        }
        else if (next)
        {
            next->setPrev(nullptr);
        }
    }
}

poBasicBlock* poFlowGraph::getBasicBlock(int index) const
{
    if (index < 0 || index >= int(_blocks.size()))
    {
        return nullptr;
    }

    return _blocks[index];
}

poBasicBlock* poFlowGraph::getLast() const
{
    return getBasicBlock(int(_blocks.size()) - 1);
}

poBasicBlock* poFlowGraph::getFirst() const
{
    return getBasicBlock(0);
}

void poFlowGraph::optimize()
{
    // Remove empty basic blocks 

    poBasicBlock* bb = getFirst();
    while (bb)
    {
        if (bb->numInstructions() == 0)
        {
            // Remove empty block
            poBasicBlock* cur = bb;
            bb = bb->getNext();
            for (auto& incoming : cur->getIncoming())
            {
                incoming->setBranch(bb, incoming->unconditionalBranch());
                bb->addIncoming(incoming);
            }
            removeBasicBlock(cur);
            delete cur;
            continue;
        }

        bb = bb->getNext();
    }

    // Remove unreachable basic blocks?
}
