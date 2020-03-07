#pragma once

#include "AstControl.h"

namespace lethe
{

class LETHE_API AstTernaryOp : public AstControl
{
public:
	SCRIPT_AST_NODE(AstTernaryOp)

	typedef AstControl Super;

	AstTernaryOp(const TokenLocation &nloc) : Super(AST_OP_TERNARY, nloc) {}

	const AstNode *GetTypeNode() const override;
	bool CodeGenRef(CompiledProgram &p, bool allowConst = false, bool derefPtr = false) override;
	bool CodeGen(CompiledProgram &p) override;
	QDataType GetTypeDesc(const CompiledProgram &p) const override;
};


}