#include "AstTernaryOp.h"
#include <Lethe/Script/Program/CompiledProgram.h>
#include <Lethe/Script/Vm/Opcodes.h>

namespace lethe
{

// AstTernaryOp

const AstNode *AstTernaryOp::GetTypeNode() const
{
	auto *t0 = nodes[1]->GetTypeNode();
	auto *t1 = nodes[2]->GetTypeNode();

	if (!t0 || !t1)
		return nullptr;

	if (t0->type == AST_CONST_NULL)
		return t1;

	if (t1->type == AST_CONST_NULL)
		return t0;

	return CoerceTypes(t0, t1);
}

QDataType AstTernaryOp::GetTypeDesc(const CompiledProgram &p) const
{
	const QDataType &dt0 = nodes[1]->GetTypeDesc(p);
	const QDataType &dt1 = nodes[2]->GetTypeDesc(p);

	// if either is null, use other type
	if (dt0.GetTypeEnum() == DT_NULL)
		return dt1;

	if (dt1.GetTypeEnum() == DT_NULL)
		return dt0;

	// should check for special types here, like delegates and so on...
	if (!dt0.IsTernaryCompatible() || !dt1.IsTernaryCompatible())
		return QDataType();

	// if types are the same, return first type
	// FIXME: what about qualifiers?!
	if (dt0.GetTypeEnum() == dt1.GetTypeEnum())
		return dt0;

	// only allow compatible structs though
	if (dt0.IsStruct() || dt1.IsStruct())
		return QDataType();

	return QDataType::MakeType(p.Coerce(dt0.GetType(), dt1.GetType()));
}

// FIXME: refactor this crap! codegenref and codegen share common path!
bool AstTernaryOp::CodeGenRef(CompiledProgram &p, bool allowConst, bool derefPtr)
{
	// [0] = cond, [1] = true_expr, [2] = false_expr

	const QDataType &dt0 = nodes[1]->GetTypeDesc(p);
	const QDataType &dt1 = nodes[2]->GetTypeDesc(p);
	const DataType &dst = p.Coerce(dt0.GetType(), dt1.GetType());

	bool voidExpr = dt0.GetTypeEnum() == DT_NONE && dt1.GetTypeEnum() == DT_NONE;

	if (!voidExpr && dst.type == DT_NONE)
		return p.Error(this, "incompatible types for ternary op");

	Int bconst = nodes[0]->ToBoolConstant(p);

	if (bconst == 1)
	{
		// true branch only
		return nodes[1]->CodeGenRef(p, allowConst, derefPtr);
	}

	if (bconst == 0)
	{
		// false branch only
		return nodes[2]->CodeGenRef(p, allowConst, derefPtr);
	}

	// first: eval condition
	LETHE_RET_FALSE(CodeGenBoolExpr(nodes[0], p));
	const auto &edt = p.exprStack.Back();

	Int fwd = p.EmitForwardJump(p.ConvJump(edt.GetTypeEnum(), OPC_IBZ_P));
	p.PopStackType(1);
	// true branch
	LETHE_RET_FALSE(nodes[1]->CodeGenRef(p, allowConst, derefPtr));

	if (!voidExpr)
	{
		if (p.exprStack.IsEmpty())
			return p.Error(nodes[1], "true expression must return a value");

		p.EmitConv(this, dt0, dst);
		p.PopStackType(1);
	}

	Int fwd2 = p.EmitForwardJump(OPC_BR);
	// false branch
	LETHE_RET_FALSE(p.FixupForwardTarget(fwd));
	LETHE_RET_FALSE(nodes[2]->CodeGenRef(p, allowConst, derefPtr));

	if (!voidExpr)
	{
		if (p.exprStack.IsEmpty())
			return p.Error(nodes[2], "false expression must return a value");

		p.EmitConv(this, dt1, dst);
		p.PopStackType(1);
	}

	LETHE_RET_FALSE(p.FixupForwardTarget(fwd2));

	if (!voidExpr)
		p.PushStackType(QDataType::MakeType(dst));

	return true;
}

bool AstTernaryOp::CodeGen(CompiledProgram &p)
{
	// [0] = cond, [1] = true_expr, [2] = false_expr

	const QDataType &dt0 = nodes[1]->GetTypeDesc(p);
	const QDataType &dt1 = nodes[2]->GetTypeDesc(p);
	const auto dst = GetTypeDesc(p);

	bool voidExpr = dt0.GetTypeEnum() == DT_NONE && dt1.GetTypeEnum() == DT_NONE;

	if (!voidExpr && dst.GetTypeEnum() == DT_NONE)
		return p.Error(this, "incompatible types for ternary op");

	Int bconst = nodes[0]->ToBoolConstant(p);

	if (bconst == 1)
	{
		// true branch only
		return nodes[1]->CodeGen(p);
	}

	if (bconst == 0)
	{
		// false branch only
		return nodes[2]->CodeGen(p);
	}

	// first: eval condition
	LETHE_RET_FALSE(CodeGenBoolExpr(nodes[0], p));
	const auto &edt = p.exprStack.Back();

	Int fwd = p.EmitForwardJump(p.ConvJump(edt.GetTypeEnum(), OPC_IBZ_P));
	p.PopStackType(1);
	// true branch
	LETHE_RET_FALSE(nodes[1]->CodeGen(p));

	if (!voidExpr)
	{
		if (p.exprStack.IsEmpty())
			return p.Error(nodes[1], "true expression must return a value");

		p.EmitConv(this, dt0, dst.GetType());
		p.PopStackType(1);
	}

	Int fwd2 = p.EmitForwardJump(OPC_BR);
	// false branch
	LETHE_RET_FALSE(p.FixupForwardTarget(fwd));
	LETHE_RET_FALSE(nodes[2]->CodeGen(p));

	if (!voidExpr)
	{
		if (p.exprStack.IsEmpty())
			return p.Error(nodes[2], "false expression must return a value");

		p.EmitConv(this, dt1, dst.GetType());
		p.PopStackType(1);
	}

	LETHE_RET_FALSE(p.FixupForwardTarget(fwd2));

	if (!voidExpr)
		p.PushStackType(dst);

	return true;
}


}
