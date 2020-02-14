#pragma once
#include <unordered_map>
#include <string>
#include <vector>

#include <aGen/aGen.hpp>

#include <ShaderDebugger/ShaderType.h>
#include <ShaderDebugger/Function.h>
#include <ShaderDebugger/Structure.h>
#include <ShaderDebugger/Variable.h>

#include <wgtcc/cpp.h>

extern "C" {
	#include <BlueVM/bv_object.h>
}

namespace sd
{
	class Compiler
	{
	public:
		inline ag::Generator& GetBytecodeGenerator() { return m_gen; }
		inline std::vector<uint8_t> GetBytecode() { return m_gen.Get().Get(); }
		inline const std::vector<Variable>& GetGlobals() { return m_globals; }
		inline const std::vector<Structure>& GetStructures() { return m_structures; }
		inline const std::vector<Function>& GetFunctions() { return m_func; }
		inline const std::vector<std::string>& GetLocals(const std::string& func) { return m_locals[func]; }
		inline const std::string& GetLocalType(const std::string& func, const std::string& var) { return m_localTypes[func][var]; }

		inline void SetImmediate(bool s) { m_isImmediate = s; }
		inline bool GetImmediate() { return m_isImmediate; }
		inline void AddFunctionDefinition(const Function& func) { m_func.push_back(func); }
		inline void AddStructureDefinition(const Structure& str) { m_structures.push_back(str); }
		inline void AddGlobalDefinition(const Variable& var) { m_globals.push_back(var); }
		inline void ClearDefinitions() { m_stringTable.clear(); m_globals.clear(); m_structures.clear(); m_func.clear(); m_locals.clear(); m_localTypes.clear(); ClearImmediate(); }

		virtual bool Parse(ShaderType type, const std::string& source, std::string entry = "main") = 0;

		virtual void ClearImmediate() = 0;
		virtual void AddImmediateGlobalDefinition(Variable var) = 0;

		inline void AddStringTableEntry(const std::string& str) { m_stringTable.push_back(str); }
		inline const std::vector<std::string>& GetStringTable() { return m_gen.GetStringTable(); }

		bv_object_get_property_ext PropertyGetter;
		bv_object_default_constructor_ext ObjectConstructor;

		enum class Language
		{
			GLSL,
			HLSL,
			Custom
		};
		inline Language GetLanguage() { return m_language; }

		inline const pp::MacroMap& GetMacroList() { return m_macros; }
		inline void AddMacro(const std::string& name, const pp::Macro& mac) {
			auto res = m_macros.find(name);
			if (res != m_macros.end()) m_macros.erase(res);
			m_macros.insert(std::make_pair(name, mac));
		}
		inline void AddMacro(const std::string& name, const std::string& src)
		{
			pp::TokenSequence ts;
			pp::Scanner scanner(&src);
			scanner.Tokenize(ts);
			AddMacro(name, pp::Macro(ts, false));
		}
		inline void ClearMacroList() { m_macros.clear(); }

	protected:
		Language m_language;
		bool m_isImmediate;
		ag::Generator m_gen;
		std::vector<Variable> m_globals;
		std::vector<Structure> m_structures;
		std::vector<Function> m_func;
		std::unordered_map<std::string, std::vector<std::string>> m_locals;
		std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_localTypes;
		std::vector<std::string> m_stringTable;

		pp::MacroMap m_macros;
	};
}