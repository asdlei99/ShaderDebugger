#pragma once
#include <ShaderDebugger/Compiler.h>
#include <ShaderDebugger/Texture.h>
#include <ShaderDebugger/Breakpoint.h>
#include <ShaderDebugger/CommonLibrary.h>
#include <glm/glm.hpp>

#include <fstream>

extern "C" {
	#include <BlueVM.h>
}

namespace sd
{
	class ShaderDebugger
	{
	public:
		ShaderDebugger();
		~ShaderDebugger();

		template <typename CodeCompiler>
		bool SetSource(sd::ShaderType stage, const std::string& src, const std::string& entry, bv_stack* args = NULL, bv_library* library = NULL)
		{
			m_clear();

			m_error = "";

			m_compiler = new CodeCompiler();
			m_compiler->SetImmediate(false);

			m_immCompiler = new CodeCompiler();
			m_immCompiler->SetImmediate(true);

			m_entry = entry;
			m_library = library;
			m_args = args;
			m_prog = nullptr;
			m_stepper = nullptr;
			m_discarded = false;

			m_type = stage;

			bool done = m_compiler->Parse(stage, src, entry);
			m_bytecode = m_compiler->GetBytecode();

			if (done && m_bytecode.size() > 0) {
				m_prog = bv_program_create(m_bytecode.data());
				if (m_prog == nullptr) {
					m_error = "Invalid bytecode.";
					return false; // invalid bytecode
				}
				m_prog->user_data = (void*)this;
				bv_program_add_function(m_prog, "$$discard", Common::Discard);

				m_prog->property_getter = m_compiler->PropertyGetter;
				m_prog->default_constructor = m_compiler->ObjectConstructor;
					
				bv_function* entryPtr = bv_program_get_function(m_prog, entry.c_str());
				if (entryPtr == nullptr) {
					m_error = "Failed to locate entry function.";
					return false;
				}

				m_stepper = bv_function_stepper_create(m_prog, entryPtr, NULL, m_args);
				
				if (m_library != nullptr)
					bv_program_add_library(m_prog, library);
			}
			
			if (!done) {
				m_error = m_compiler->GetLastErrorMessage();
				return false;
			}

			return true;
		}

		inline Compiler* GetCompiler() { return m_compiler; }

		inline bv_variable Execute() { return Execute(m_entry, m_args); }
		bv_variable Execute(const std::string& func, bv_stack* args = NULL); // TODO: arguments

		inline bv_variable GetReturnValue() { return bv_variable_copy(m_stepper->result); }

		void SetArguments(bv_stack* args);

		std::string GetCurrentFunction();
		std::vector<std::string> GetFunctionStack();
		std::vector<std::string> GetCurrentFunctionLocals();
		bv_variable* GetLocalValue(const std::string& varname);
		int GetCurrentLine() { return m_prog->current_line; }
		void Jump(int line);
		bool Continue();
		bool Step();
		bool StepOver();
		bool StepOut();
		bool HasBreakpoint(int ln);
		void AddBreakpoint(int ln);
		void AddConditionalBreakpoint(int ln, std::string condition);
		void ClearBreakpoint(int ln);
		inline void ClearBreakpoints() { m_breakpoints.clear(); }
		bv_variable Immediate(const std::string& command);

		// semantics
		void SetSemanticValue(const std::string& name, bv_variable var);
		bv_variable GetSemanticValue(const std::string& name);

		// for more complex types, we need to provide classType (for example, vec3 is for GLSL but float3 is used in HLSL)
		// this makes ShaderDebugger work without needing to know which shader language it uses
		void SetGlobalValue(const std::string& varName, bv_variable value);
		void SetGlobalValue(const std::string& varName, float value);
		bool SetGlobalValue(const std::string& varName, const std::string& classType, glm::vec4 val);
		bool SetGlobalValue(const std::string& varName, const std::string& classType, sd::Texture* val);
		bool SetGlobalValue(const std::string& varName, const std::string& classType, glm::mat4 val);
		template <size_t c, typename t>
		bool SetGlobalValue(const std::string& varName, const std::string& classType, glm::vec<c, t, glm::defaultp> val)
		{
			glm::vec4 actualVal(0.0f);
			for (size_t i = 0; i < c; i++)
				actualVal[i] = val[i];
			SetGlobalValue(varName, classType, actualVal);
		}

		bv_variable* GetGlobalValue(const std::string& gvarname);

		void AddGlobal(const std::string& varName);

		inline void SetDiscarded(bool d) { 
			m_discarded = d;

			if (d) {
				bv_function_stepper_abort(m_stepper);
				bv_program_abort(m_prog);
			}
		}
		inline bool IsDiscarded() { return m_discarded; }
		inline std::string GetLastError() { return m_error; }

		inline bv_program* GetProgram() { return m_prog; }

	private:
		bool m_checkBreakpoint(int line);
		Function m_getFunctionInfo(const std::string& fname);

		std::string m_typeToString(const bv_variable& var);
		void m_clear();

		bool m_discarded;

		std::vector<Breakpoint> m_breakpoints;

		std::string m_error;

		sd::ShaderType m_type;
		Compiler* m_compiler, *m_immCompiler;
		std::string m_entry;
		bv_library* m_library;
		bv_program* m_prog;
		bv_stack* m_args;
		bv_function_stepper* m_stepper;
		std::vector<uint8_t> m_bytecode;

		std::unordered_map<std::string, bv_variable> m_semantics;
	};
}