/*
 * Copyright 2019 Hans-Kristian Arntzen for Valve Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#pragma once

#include "dxil_converter.hpp"
#include "SpvBuilder.h"
#include "cfg_structurizer.hpp"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Instructions.h>
#include "GLSL.std.450.h"

namespace DXIL2SPIRV
{
struct Converter::Impl
{
	Impl(DXILContainerParser container_parser_, LLVMBCParser bitcode_parser_, SPIRVModule &module_)
			: container_parser(std::move(container_parser_))
			, bitcode_parser(std::move(bitcode_parser_))
			, spirv_module(module_)
	{
	}

	DXILContainerParser container_parser;
	LLVMBCParser bitcode_parser;
	SPIRVModule &spirv_module;

	struct BlockMeta
	{
		explicit BlockMeta(llvm::BasicBlock *bb_)
				: bb(bb_)
		{
		}

		llvm::BasicBlock *bb;
		CFGNode *node = nullptr;
	};
	std::vector<std::unique_ptr<BlockMeta>> metas;
	std::unordered_map<llvm::BasicBlock *, BlockMeta *> bb_map;
	std::unordered_map<const llvm::Value *, spv::Id> value_map;

	ConvertedFunction convert_entry_point();
	spv::Id get_id_for_value(const llvm::Value *value, unsigned forced_integer_width = 0);
	spv::Id get_id_for_constant(const llvm::Constant *constant, unsigned forced_width);
	spv::Id get_id_for_undef(const llvm::UndefValue *undef);

	void emit_stage_input_variables();
	void emit_stage_output_variables();
	void emit_interpolation_decorations(spv::Id variable_id, DXIL::InterpolationMode mode);

	void emit_resources();
	void emit_srvs(const llvm::MDNode *srvs);
	void emit_uavs(const llvm::MDNode *uavs);
	void emit_cbvs(const llvm::MDNode *cbvs);
	void emit_samplers(const llvm::MDNode *samplers);

	std::vector<spv::Id> srv_index_to_id;
	std::vector<spv::Id> uav_index_to_id;
	std::vector<spv::Id> cbv_index_to_id;
	std::vector<spv::Id> sampler_index_to_id;
	std::unordered_map<const llvm::Value *, spv::Id> handle_to_ptr_id;
	std::unordered_map<spv::Id, spv::Id> id_to_type;

	spv::Id get_type_id(DXIL::ComponentType element_type, unsigned rows, unsigned cols);
	spv::Id get_type_id(const llvm::Type *type);
	spv::Id get_type_id(spv::Id id) const;

	struct ElementMeta
	{
		spv::Id id;
		DXIL::ComponentType component_type;
	};
	std::unordered_map<uint32_t, ElementMeta> input_elements_meta;
	std::unordered_map<uint32_t, ElementMeta> output_elements_meta;
	void emit_builtin_decoration(spv::Id id, DXIL::Semantic semantic);

	bool emit_instruction(CFGNode *block, const llvm::Instruction &instruction);
	bool emit_phi_instruction(CFGNode *block, const llvm::PHINode &instruction);

	// DXIL intrinsics.
	bool emit_builtin_instruction(CFGNode *block, const llvm::CallInst &instruction);

	void emit_load_input_instruction(CFGNode *block, const llvm::CallInst &instruction);
	void emit_store_output_instruction(CFGNode *block, const llvm::CallInst &instruction);
	void emit_create_handle_instruction(CFGNode *block, const llvm::CallInst &instruction);
	void emit_cbuffer_load_legacy_instruction(CFGNode *block, const llvm::CallInst &instruction);
	void emit_sample_instruction(DXIL::Op op, CFGNode *block, const llvm::CallInst &instruction);
	void emit_saturate_instruction(CFGNode *block, const llvm::CallInst &instruction);

	void emit_dxil_unary_instruction(spv::Op op, CFGNode *block, const llvm::CallInst &instruction);
	void emit_dxil_std450_unary_instruction(GLSLstd450 op, CFGNode *block, const llvm::CallInst &instruction);
	void emit_dxil_std450_binary_instruction(GLSLstd450 op, CFGNode *block, const llvm::CallInst &instruction);

	static uint32_t get_constant_operand(const llvm::CallInst &value, unsigned index);
	spv::Id build_sampled_image(CFGNode *block, spv::Id image_id, spv::Id sampler_id, bool comparison);
	spv::Id build_vector(CFGNode *block, spv::Id element_type, spv::Id *elements, unsigned count);

	spv::Id glsl_std450_ext = 0;
};
}