// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_CODEEMITTER_H
#define _ASMJIT_BASE_CODEEMITTER_H

// [Dependencies]
#include "../base/codeholder.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

class ConstPool;

// ============================================================================
// [asmjit::CodeEmitter]
// ============================================================================

//! Provides a base foundation to emit code - specialized by \ref Assembler and
//! \ref CodeBuilder.
class ASMJIT_VIRTAPI CodeEmitter {
public:
  //! CodeEmitter type.
  ASMJIT_ENUM(Type) {
    kTypeNone       = 0,
    kTypeAssembler  = 1,
    kTypeBuilder    = 2,
    kTypeCompiler   = 3,
    kTypeCount      = 4
  };

  //! CodeEmitter hints - global settings that affect machine-code generation.
  ASMJIT_ENUM(Hints) {
    //! Emit optimized code-alignment sequences.
    //!
    //! Default `true`.
    //!
    //! X86/X64 Specific
    //! ----------------
    //!
    //! Default align sequence used by X86/X64 architecture is one-byte (0x90)
    //! opcode that is often shown by disassemblers as nop. However there are
    //! more optimized align sequences for 2-11 bytes that may execute faster.
    //! If this feature is enabled AsmJit will generate specialized sequences
    //! for alignment between 1 to 11 bytes. Also when `X86Compiler` is used,
    //! it can add REX prefixes into the code to make some instructions greater
    //! so no alignment sequence is needed.
    kHintOptimizedAlign = 0x00000001U,

    //! Emit jump-prediction hints.
    //!
    //! Default `false`.
    //!
    //! X86/X64 Specific
    //! ----------------
    //!
    //! Jump prediction is usually based on the direction of the jump. If the
    //! jump is backward it is usually predicted as taken; and if the jump is
    //! forward it is usually predicted as not-taken. The reason is that loops
    //! generally use backward jumps and conditions usually use forward jumps.
    //! However this behavior can be overridden by using instruction prefixes.
    //! If this option is enabled these hints will be emitted.
    //!
    //! This feature is disabled by default, because the only processor that
    //! used to take into consideration prediction hints was P4. Newer processors
    //! implement heuristics for branch prediction that ignores any static hints.
    kHintPredictedJumps = 0x00000002U
  };

  //! CodeEmitter options that are merged with instruction options.
  ASMJIT_ENUM(Options) {
    //! Reserved, used to check for errors in `Assembler::_emit()`.
    //!
    //! The `kOptionMaybeFailureCase` is always set when CodeEmitter is in
    //! error state.
    kOptionMaybeFailureCase = 0x00000001U,

    //! Perform a strict validation before the instruction is emitted.
    //!
    //! NOTE: This option is used by both Assembler and CodeCompiler.
    kOptionStrictValidation = 0x00000002U,

    //! Logging is enabled and `CodeHolder::getLogger()` should return a valid
    //! \ref Logger pointer.
    kOptionLoggingEnabled   = 0x00000010U,

    //! Mask of all internal options that are not used to represent instruction
    //! options, but are used to instrument Assembler and CodeBuilder. These
    //! options are internal and should not be used outside of AsmJit itself.
    kOptionReservedMask = 0x00000013U,

    //! Instruction has `_op4` (5th operand, indexed from zero).
    kOptionHasOp4 = 0x0000020U,
    //! Instruction has `_op5` (6th operand, indexed from zero).
    kOptionHasOp5 = 0x0000040U,
    //! Instruction has mask-op (k) operand.
    kOptionHasOpMask = 0x00000080U,

    //! Don't follow the jump (CodeCompiler).
    //!
    //! Prevents following the jump during compilation (CodeCompiler).
    kOptionUnfollow = 0x00000100U,

    //! Overwrite the destination operand (CodeCompiler).
    //!
    //! Hint that is important for register liveness analysis. It tells the
    //! compiler that the destination operand will be overwritten now or by
    //! adjacent instructions. CodeCompiler knows when a register is completely
    //! overwritten by a single instruction, for example you don't have to
    //! mark "movaps" or "pxor x, x", however, if a pair of instructions is
    //! used and the first of them doesn't completely overwrite the content
    //! of the destination, CodeCompiler fails to mark that register as dead.
    //!
    //! X86/X64 Specific
    //! ----------------
    //!
    //!   - All instructions that always overwrite at least the size of the
    //!     register the virtual-register uses , for example "mov", "movq",
    //!     "movaps" don't need the overwrite option to be used - conversion,
    //!     shuffle, and other miscellaneous instructions included.
    //!
    //!   - All instructions that clear the destination register if all operands
    //!     are the same, for example "xor x, x", "pcmpeqb x x", etc...
    //!
    //!   - Consecutive instructions that partially overwrite the variable until
    //!     there is no old content require the `overwrite()` to be used. Some
    //!     examples (not always the best use cases thought):
    //!
    //!     - `movlps xmm0, ?` followed by `movhps xmm0, ?` and vice versa
    //!     - `movlpd xmm0, ?` followed by `movhpd xmm0, ?` and vice versa
    //!     - `mov al, ?` followed by `and ax, 0xFF`
    //!     - `mov al, ?` followed by `mov ah, al`
    //!     - `pinsrq xmm0, ?, 0` followed by `pinsrq xmm0, ?, 1`
    //!
    //!   - If allocated variable is used temporarily for scalar operations. For
    //!     example if you allocate a full vector like `X86Compiler::newXmm()`
    //!     and then use that vector for scalar operations you should use
    //!     `overwrite()` directive:
    //!
    //!     - `sqrtss x, y` - only LO element of `x` is changed, if you don't use
    //!       HI elements, use `X86Compiler.overwrite().sqrtss(x, y)`.
    kOptionOverwrite = 0x00000200U
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_API CodeEmitter(uint32_t type) noexcept;
  ASMJIT_API virtual ~CodeEmitter() noexcept;

  // --------------------------------------------------------------------------
  // [Events]
  // --------------------------------------------------------------------------

  //! Called after the \ref CodeEmitter was attached to the \ref CodeHolder.
  virtual Error onAttach(CodeHolder* code) noexcept = 0;
  //! Called after the \ref CodeEmitter was detached from the \ref CodeHolder.
  virtual Error onDetach(CodeHolder* code) noexcept = 0;

  // --------------------------------------------------------------------------
  // [Code-Generation]
  // --------------------------------------------------------------------------

  //! Emit instruction.
  virtual Error _emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) = 0;

  //! Create new `Label`.
  virtual Label newLabel() = 0;

  //! Bind the `label` to the current position of the current section.
  //!
  //! NOTE: Attempt to bind the same label multiple times will return an error.
  virtual Error bind(const Label& label) = 0;

  //! Align to the `alignment` specified.
  //!
  //! The sequence that is used to fill the gap between the aligned location
  //! and the current location depends on the align `mode`, see \ref AlignMode.
  virtual Error align(uint32_t mode, uint32_t alignment) = 0;

  //! Embed raw data into the code-buffer.
  virtual Error embed(const void* data, uint32_t size) = 0;

  //! Embed a constant pool into the code-buffer in the following steps:
  //!   1. Align by using kAlignData to the minimum `pool` alignment.
  //!   2. Bind `label` so it's bound to an aligned location.
  //!   3. Emit constant pool data.
  virtual Error embedConstPool(const Label& label, const ConstPool& pool) = 0;

  //! Emit a comment string `s` with an optional `len` parameter.
  virtual Error comment(const char* s, size_t len = kInvalidIndex) = 0;

  // --------------------------------------------------------------------------
  // [Code-Generation Status]
  // --------------------------------------------------------------------------

  //! Get if the CodeEmitter is initialized (i.e. attached to a \ref CodeHolder).
  ASMJIT_INLINE bool isInitialized() const noexcept { return _code != nullptr; }

  ASMJIT_API virtual Error finalize();

  // --------------------------------------------------------------------------
  // [Code Information]
  // --------------------------------------------------------------------------

  //! Get information about the code, see \ref CodeInfo.
  ASMJIT_INLINE const CodeInfo& getCodeInfo() const noexcept { return _codeInfo; }
  //! Get \ref CodeHolder this CodeEmitter is attached to.
  ASMJIT_INLINE CodeHolder* getCode() const noexcept { return _code; }

  //! Get information about the architecture, see \ref Arch.
  ASMJIT_INLINE const Arch& getArch() const noexcept { return _codeInfo._arch; }
  //! Get the target architecture.
  ASMJIT_INLINE uint32_t getArchType() const noexcept { return _codeInfo._arch.getType(); }
  //! Get the target architecture's GP register size (4 or 8 bytes).
  ASMJIT_INLINE uint32_t getGpSize() const noexcept { return _codeInfo._arch.getGpSize(); }
  //! Get the number of target GP registers.
  ASMJIT_INLINE uint32_t getGpCount() const noexcept { return _codeInfo._arch.getGpCount(); }

  // --------------------------------------------------------------------------
  // [Global Information]
  // --------------------------------------------------------------------------

  //! Get global hints.
  ASMJIT_INLINE uint32_t getGlobalHints() const noexcept { return _globalHints; }

  //! Get global options.
  //!
  //! Global options are merged with instruction options before the instruction
  //! is encoded. These options have some bits reserved that are used for error
  //! checking, logging, and strict validation. Other options are globals that
  //! affect each instruction, for example if VEX3 is set globally, it will all
  //! instructions, even those that don't have such option set.
  ASMJIT_INLINE uint32_t getGlobalOptions() const noexcept { return _globalOptions; }

  // --------------------------------------------------------------------------
  // [Code-Emitter Information]
  // --------------------------------------------------------------------------

  //! Get the type of this CodeEmitter, see \ref Type.
  ASMJIT_INLINE uint32_t getType() const noexcept { return _type; }

  // --------------------------------------------------------------------------
  // [Error Handling]
  // --------------------------------------------------------------------------

  //! Get if the object is in error state.
  //!
  //! Error state means that it does not consume anything unless the error
  //! state is reset by calling `resetLastError()`. Use `getLastError()` to
  //! get the last error that put the object into the error state.
  ASMJIT_INLINE bool isInErrorState() const noexcept { return _lastError != kErrorOk; }

  //! Get the last error code.
  ASMJIT_INLINE Error getLastError() const noexcept { return _lastError; }
  //! Set the last error code and propagate it through the error handler.
  ASMJIT_API Error setLastError(Error error, const char* message = nullptr);
  //! Clear the last error code.
  ASMJIT_INLINE void resetLastError() noexcept { setLastError(kErrorOk); }

  // --------------------------------------------------------------------------
  // [Accessors That Affect the Next Instruction]
  // --------------------------------------------------------------------------

  //! Get options of the next instruction.
  ASMJIT_INLINE uint32_t getOptions() const noexcept { return _options; }
  //! Set options of the next instruction.
  ASMJIT_INLINE void setOptions(uint32_t options) noexcept { _options = options; }
  //! Add options of the next instruction.
  ASMJIT_INLINE void addOptions(uint32_t options) noexcept { _options |= options; }
  //! Reset options of the next instruction.
  ASMJIT_INLINE void resetOptions() noexcept { _options = 0; }

  //! Get if the 5th operand (indexed from zero) of the next instruction is used.
  ASMJIT_INLINE bool hasOp4() const noexcept { return (_options & kOptionHasOp4) != 0; }
  //! Get if the 6th operand (indexed from zero) of the next instruction is used.
  ASMJIT_INLINE bool hasOp5() const noexcept { return (_options & kOptionHasOp5) != 0; }
  //! Get if the op-mask operand of the next instruction is used.
  ASMJIT_INLINE bool hasOpMask() const noexcept { return (_options & kOptionHasOpMask) != 0; }

  ASMJIT_INLINE const Operand& getOp4() const noexcept { return static_cast<const Operand&>(_op4); }
  ASMJIT_INLINE const Operand& getOp5() const noexcept { return static_cast<const Operand&>(_op5); }
  ASMJIT_INLINE const Operand& getOpMask() const noexcept { return static_cast<const Operand&>(_opMask); }

  ASMJIT_INLINE void setOp4(const Operand_& op4) noexcept { _options |= kOptionHasOp4; _op4 = op4; }
  ASMJIT_INLINE void setOp5(const Operand_& op5) noexcept { _options |= kOptionHasOp5; _op5 = op5; }
  ASMJIT_INLINE void setOpMask(const Operand_& opMask) noexcept { _options |= kOptionHasOpMask; _opMask = opMask; }

  //! Get annotation of the next instruction.
  ASMJIT_INLINE const char* getInlineComment() const noexcept { return _inlineComment; }
  //! Set annotation of the next instruction.
  //!
  //! NOTE: This string is set back to null by `_emit()`, but until that it has
  //! to remain valid as `CodeEmitter` is not required to make a copy of it (and
  //! it would be slow to do that for each instruction).
  ASMJIT_INLINE void setInlineComment(const char* s) noexcept { _inlineComment = s; }
  //! Reset annotation of the next instruction to null.
  ASMJIT_INLINE void resetInlineComment() noexcept { _inlineComment = nullptr; }

  // --------------------------------------------------------------------------
  // [Helpers]
  // --------------------------------------------------------------------------

  //! Get if the `label` is valid (i.e. registered).
  ASMJIT_INLINE bool isLabelValid(const Label& label) const noexcept {
    return isLabelValid(label.getId());
  }

  //! Get if the label `id` is valid (i.e. registered).
  ASMJIT_API bool isLabelValid(uint32_t id) const noexcept;

  //! Emit a formatted string `fmt`.
  ASMJIT_API Error commentf(const char* fmt, ...);
  //! Emit a formatted string `fmt` (va_list version).
  ASMJIT_API Error commentv(const char* fmt, va_list ap);

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  // NOTE: These `emit()` helpers are designed to address a code-bloat generated
  // by C++ compilers to call a function having many arguments. Each parameter to
  // `_emit()` requires code to pass it, which means that if we default to 4
  // operand parameters in `_emit()` and instId the C++ compiler would have to
  // generate a virtual function call having 5 parameters, which is quite a lot.
  // Since by default asm instructions have 2 to 3 operands it's better to
  // introduce helpers that pass those and fill all the remaining with `_none`.

  //! Emit an instruction.
  ASMJIT_API Error emit(uint32_t instId);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0, const Operand_& o1);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3, const Operand_& o4);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3, const Operand_& o4, const Operand_& o5);

  //! Emit an instruction that has a 32-bit signed immediate operand.
  ASMJIT_API Error emit(uint32_t instId, int o0);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0, int o1);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0, const Operand_& o1, int o2);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, int o3);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3, int o4);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3, const Operand_& o4, int o5);

  //! Emit an instruction that has a 64-bit signed immediate operand.
  ASMJIT_API Error emit(uint32_t instId, int64_t o0);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0, int64_t o1);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0, const Operand_& o1, int64_t o2);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, int64_t o3);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3, int64_t o4);
  //! \overload
  ASMJIT_API Error emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3, const Operand_& o4, int64_t o5);

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  CodeInfo _codeInfo;                    //!< Basic information about the code (matches CodeHolder::_codeInfo).
  CodeHolder* _code;                     //!< CodeHolder.
  CodeEmitter* _nextEmitter;             //!< Linked list of `CodeEmitter`s attached to a single \ref CodeHolder.

  uint8_t _type;                         //!< See CodeEmitter::Type.
  uint8_t _destroyed;                    //!< Set by ~CodeEmitter() before calling `_code->detach()`.
  uint8_t _finalized;                    //!< True if the CodeEmitter is finalized (CodeBuilder & CodeCompiler).
  uint8_t _reserved;                     //!< \internal
  Error _lastError;                      //!< Last error code.

  uint32_t _privateData;                 //!< Internal private data used freely by any CodeEmitter.
  uint32_t _globalHints;                 //!< Global hints, always in sync with CodeHolder.
  uint32_t _globalOptions;               //!< Global options, combined with `_options` before used by each instruction.

  uint32_t _options;                     //!< Used to pass instruction options       (affects the next instruction).
  const char* _inlineComment;            //!< Inline comment of the next instruction (affects the next instruction).
  Operand_ _op4;                         //!< 5th operand data (indexed from zero)   (affects the next instruction).
  Operand_ _op5;                         //!< 6th operand data (indexed from zero)   (affects the next instruction).
  Operand_ _opMask;                      //!< op-mask (k) operand data               (affects the next instruction).
  Operand_ _none;                        //!< Used to pass unused operands to `_emit()` instead of passing null.
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_CODEEMITTER_H