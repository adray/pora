#include "poCFG.h"
#include <algorithm>
#include <assert.h>

using namespace po;

//======================
// poInstruction
//======================

poInstruction::poInstruction(const int32_t name, const int16_t type, const int16_t left, const int16_t right, const int16_t code)
    :
    _type(type),
    _left(left),
    _right(right),
    _memOffset(0),
    _code(code),
    _name(name)
{
    assert(type >= 0);
}

poInstruction::poInstruction(const int32_t name, const int16_t type, const int16_t constant, const int16_t code)
    :
    _type(type),
    _constant(constant),
    _code(code),
    _left(-1),
    _right(-1),
    _name(name)
{
    assert(type >= 0);
}

poInstruction::poInstruction(const int32_t name, const int16_t type, const int16_t left, const int16_t right, const int16_t memOffset, const int16_t code)
    :
    _type(type),
    _left(left),
    _right(right),
    _memOffset(memOffset),
    _code(code),
    _name(name)
{
    assert(type >= 0);
}

bool poInstruction::isSpecialInstruction() const
{
    // 
    // We deem these special instructions if the left or right
    // parameters are reused for something else, other than
    // referring to a use of another instruction.

    switch (_code)
    {
    case (IR_CONSTANT): /* technically not, but keep it in for now */
    case (IR_CALL):
    case (IR_BR):
    case (IR_PARAM):
    case (IR_ALLOCA):
    case (IR_MALLOC):
        return true;
    }
    return false;
}

//====================
// poInstructionRef
//====================

poInstructionRef::poInstructionRef(poBasicBlock* bb, const int ref, const int baseRef)
    :
    _bb(bb),
    _ref(ref),
    _baseRef(baseRef)
{
}

poInstruction& poInstructionRef::getInstruction() const
{
    return _bb->getInstruction(_ref - _baseRef);
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

//==============
// poSSAPhi
//==============

poPhi::poPhi(const int name, const int type)
    :
    _name(name),
    _initialName(name),
    _type(type)
{
}

void poPhi::addValue(const int value, poBasicBlock* bb)
{
    _rhs.push_back(value);
    _bb.push_back(bb);
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

void poFlowGraph::insertBasicBlock(poBasicBlock* insertAfter, poBasicBlock* bb)
{
    const auto& it = std::find(_blocks.begin(), _blocks.end(), insertAfter);
    if (it != _blocks.end())
    {
        _blocks.insert(it+1, bb);

        bb->setNext(insertAfter->getNext());
        if (insertAfter->getNext())
        {
            insertAfter->getNext()->setPrev(bb);
        }
        insertAfter->setNext(bb);
        bb->setPrev(insertAfter);
    }
}

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
            if (cur->getIncoming().size() == 0 &&
                cur->getPrev())
            {
                bb = bb->getNext();

                if (cur->getPrev()->getBranch() && !cur->getPrev()->unconditionalBranch())
                {
                    continue;
                }

                removeBasicBlock(cur);
                delete cur;
                continue;
            }
        }

        bb = bb->getNext();
    }

    // Remove unreachable basic blocks?
}

//poFlowGraph::~poFlowGraph()
//{
//    for (poBasicBlock* bb : _blocks)
//    {
//        delete bb;
//    }
//    _blocks.clear();
//}
