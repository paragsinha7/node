// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_ELEMENTS_H_
#define V8_OBJECTS_ELEMENTS_H_

#include "src/builtins/builtins-utils.h"
#include "src/objects/elements-kind.h"
#include "src/objects/internal-index.h"
#include "src/objects/keys.h"
#include "src/objects/objects.h"

namespace v8 {
namespace internal {

class JSTypedArray;

// Abstract base class for handles that can operate on objects with differing
// ElementsKinds.
class ElementsAccessor {
 public:
  ElementsAccessor() = default;
  virtual ~ElementsAccessor() = default;
  ElementsAccessor(const ElementsAccessor&) = delete;
  ElementsAccessor& operator=(const ElementsAccessor&) = delete;

  // Returns a shared ElementsAccessor for the specified ElementsKind.
  static ElementsAccessor* ForKind(ElementsKind elements_kind) {
    DCHECK_LT(static_cast<int>(elements_kind), kElementsKindCount);
    return elements_accessors_[elements_kind];
  }

  // Checks the elements of an object for consistency, asserting when a problem
  // is found.
  virtual void Validate(Isolate* isolate, Tagged<JSObject> obj) = 0;

  // Returns true if a holder contains an element with the specified index
  // without iterating up the prototype chain. The first version takes the
  // backing store to use for the check, which must be compatible with the
  // ElementsKind of the ElementsAccessor; the second version uses
  // holder->elements() as the backing store. If a |filter| is specified,
  // the PropertyAttributes of the element at the given index are compared
  // to the given |filter|. If they match/overlap, the given index is ignored.
  // Note that only Dictionary elements have custom
  // PropertyAttributes associated, hence the |filter| argument is ignored for
  // all but DICTIONARY_ELEMENTS and SLOW_SLOPPY_ARGUMENTS_ELEMENTS.
  virtual bool HasElement(Isolate* isolate, Tagged<JSObject> holder,
                          uint32_t index, Tagged<FixedArrayBase> backing_store,
                          PropertyFilter filter = ALL_PROPERTIES) = 0;

  inline bool HasElement(Isolate* isolate, Tagged<JSObject> holder,
                         uint32_t index,
                         PropertyFilter filter = ALL_PROPERTIES);

  // Note: this is currently not implemented for string wrapper and
  // typed array elements.
  virtual bool HasEntry(Isolate* isolate, Tagged<JSObject> holder,
                        InternalIndex entry) = 0;

  virtual Handle<Object> Get(Isolate* isolate, DirectHandle<JSObject> holder,
                             InternalIndex entry) = 0;

  // Currently only shared array elements support sequentially consistent
  // access.
  virtual Handle<Object> GetAtomic(Isolate* isolate,
                                   DirectHandle<JSObject> holder,
                                   InternalIndex entry,
                                   SeqCstAccessTag tag) = 0;

  virtual bool HasAccessors(Tagged<JSObject> holder) = 0;
  virtual size_t NumberOfElements(Isolate* isolate,
                                  Tagged<JSObject> holder) = 0;

  // Modifies the length data property as specified for JSArrays and resizes the
  // underlying backing store accordingly. The method honors the semantics of
  // changing array sizes as defined in ECMAScript 5.1 15.4.5.2, i.e. array that
  // have non-deletable elements can only be shrunk to the size of highest
  // element that is non-deletable.
  V8_WARN_UNUSED_RESULT virtual Maybe<bool> SetLength(
      Isolate* isolate, DirectHandle<JSArray> holder, uint32_t new_length) = 0;

  // Copy all indices that have elements from |object| into the given
  // KeyAccumulator. For Dictionary-based element-kinds we filter out elements
  // whose PropertyAttribute match |filter|.
  V8_WARN_UNUSED_RESULT virtual ExceptionStatus CollectElementIndices(
      DirectHandle<JSObject> object, DirectHandle<FixedArrayBase> backing_store,
      KeyAccumulator* keys) = 0;

  V8_WARN_UNUSED_RESULT inline ExceptionStatus CollectElementIndices(
      DirectHandle<JSObject> object, KeyAccumulator* keys);

  virtual Maybe<bool> CollectValuesOrEntries(
      Isolate* isolate, DirectHandle<JSObject> object,
      DirectHandle<FixedArray> values_or_entries, bool get_entries,
      int* nof_items, PropertyFilter filter = ALL_PROPERTIES) = 0;

  virtual MaybeHandle<FixedArray> PrependElementIndices(
      Isolate* isolate, DirectHandle<JSObject> object,
      DirectHandle<FixedArrayBase> backing_store, DirectHandle<FixedArray> keys,
      GetKeysConversion convert, PropertyFilter filter = ALL_PROPERTIES) = 0;

  inline MaybeHandle<FixedArray> PrependElementIndices(
      Isolate* isolate, DirectHandle<JSObject> object,
      DirectHandle<FixedArray> keys, GetKeysConversion convert,
      PropertyFilter filter = ALL_PROPERTIES);

  V8_WARN_UNUSED_RESULT virtual ExceptionStatus AddElementsToKeyAccumulator(
      DirectHandle<JSObject> receiver, KeyAccumulator* accumulator,
      AddKeyConversion convert) = 0;

  virtual void TransitionElementsKind(Isolate* isolate,
                                      DirectHandle<JSObject> object,
                                      DirectHandle<Map> map) = 0;
  V8_WARN_UNUSED_RESULT virtual Maybe<bool> GrowCapacityAndConvert(
      Isolate* isolate, DirectHandle<JSObject> object, uint32_t capacity) = 0;
  // Unlike GrowCapacityAndConvert do not attempt to convert the backing store
  // and simply return false in this case.
  V8_WARN_UNUSED_RESULT virtual Maybe<bool> GrowCapacity(
      Isolate* isolate, DirectHandle<JSObject> object, uint32_t index) = 0;

  static void InitializeOncePerProcess();
  static void TearDown();

  virtual void Set(DirectHandle<JSObject> holder, InternalIndex entry,
                   Tagged<Object> value) = 0;

  // Currently only shared array elements support sequentially consistent
  // access.
  virtual void SetAtomic(DirectHandle<JSObject> holder, InternalIndex entry,
                         Tagged<Object> value, SeqCstAccessTag tag) = 0;

  // Currently only shared array elements support sequentially consistent
  // access.
  virtual Handle<Object> SwapAtomic(Isolate* isolate,
                                    DirectHandle<JSObject> holder,
                                    InternalIndex entry, Tagged<Object> value,
                                    SeqCstAccessTag tag) = 0;

  virtual Handle<Object> CompareAndSwapAtomic(
      Isolate* isolate, DirectHandle<JSObject> holder, InternalIndex entry,
      Tagged<Object> expected, Tagged<Object> value, SeqCstAccessTag tag) = 0;

  V8_WARN_UNUSED_RESULT virtual Maybe<bool> Add(Isolate* isolate,
                                                DirectHandle<JSObject> object,
                                                uint32_t index,
                                                DirectHandle<Object> value,
                                                PropertyAttributes attributes,
                                                uint32_t new_capacity) = 0;

  static DirectHandle<JSArray> Concat(Isolate* isolate, BuiltinArguments* args,
                                      uint32_t concat_size,
                                      uint32_t result_length);

  V8_WARN_UNUSED_RESULT virtual Maybe<uint32_t> Push(
      Isolate* isolate, DirectHandle<JSArray> receiver, BuiltinArguments* args,
      uint32_t push_size) = 0;

  V8_WARN_UNUSED_RESULT virtual Maybe<uint32_t> Unshift(
      Isolate* isolate, DirectHandle<JSArray> receiver, BuiltinArguments* args,
      uint32_t unshift_size) = 0;

  V8_WARN_UNUSED_RESULT virtual MaybeDirectHandle<Object> Pop(
      Isolate* isolate, DirectHandle<JSArray> receiver) = 0;

  V8_WARN_UNUSED_RESULT virtual MaybeDirectHandle<Object> Shift(
      Isolate* isolate, DirectHandle<JSArray> receiver) = 0;

  virtual DirectHandle<NumberDictionary> Normalize(
      Isolate* isolate, DirectHandle<JSObject> object) = 0;

  virtual size_t GetCapacity(Tagged<JSObject> holder,
                             Tagged<FixedArrayBase> backing_store) = 0;

  V8_WARN_UNUSED_RESULT virtual MaybeDirectHandle<Object> Fill(
      Isolate* isolate, DirectHandle<JSObject> receiver,
      DirectHandle<Object> obj_value, size_t start, size_t end) = 0;

  // Check an Object's own elements for an element (using SameValueZero
  // semantics)
  virtual Maybe<bool> IncludesValue(Isolate* isolate,
                                    DirectHandle<JSObject> receiver,
                                    DirectHandle<Object> value, size_t start,
                                    size_t length) = 0;

  // Check an Object's own elements for the index of an element (using SameValue
  // semantics)
  virtual Maybe<int64_t> IndexOfValue(Isolate* isolate,
                                      DirectHandle<JSObject> receiver,
                                      DirectHandle<Object> value, size_t start,
                                      size_t length) = 0;

  virtual Maybe<int64_t> LastIndexOfValue(DirectHandle<JSObject> receiver,
                                          DirectHandle<Object> value,
                                          size_t start) = 0;

  virtual void Reverse(Tagged<JSObject> receiver) = 0;

  virtual void CopyElements(Isolate* isolate,
                            DirectHandle<FixedArrayBase> source,
                            ElementsKind source_kind,
                            DirectHandle<FixedArrayBase> destination,
                            int size) = 0;

  virtual Tagged<Object> CopyElements(Isolate* isolate,
                                      DirectHandle<JSAny> source,
                                      DirectHandle<JSObject> destination,
                                      size_t length, size_t offset) = 0;

  virtual Handle<FixedArray> CreateListFromArrayLike(
      Isolate* isolate, DirectHandle<JSObject> object, uint32_t length) = 0;

  virtual void CopyTypedArrayElementsSlice(Tagged<JSTypedArray> source,
                                           Tagged<JSTypedArray> destination,
                                           size_t start, size_t end) = 0;

 protected:
  friend class LookupIterator;

  // Element handlers distinguish between entries and indices when they
  // manipulate elements. Entries refer to elements in terms of their location
  // in the underlying storage's backing store representation, and are between 0
  // and GetCapacity. Indices refer to elements in terms of the value that would
  // be specified in JavaScript to access the element. In most implementations,
  // indices are equivalent to entries. In the NumberDictionary
  // ElementsAccessor, entries are mapped to an index using the KeyAt method on
  // the NumberDictionary.
  virtual InternalIndex GetEntryForIndex(Isolate* isolate,
                                         Tagged<JSObject> holder,
                                         Tagged<FixedArrayBase> backing_store,
                                         size_t index) = 0;

  virtual PropertyDetails GetDetails(Tagged<JSObject> holder,
                                     InternalIndex entry) = 0;
  virtual void Reconfigure(Isolate* isolate, DirectHandle<JSObject> object,
                           DirectHandle<FixedArrayBase> backing_store,
                           InternalIndex entry, DirectHandle<Object> value,
                           PropertyAttributes attributes) = 0;

  // Deletes an element in an object.
  virtual void Delete(Isolate* isolate, DirectHandle<JSObject> holder,
                      InternalIndex entry) = 0;

  // NOTE: this method violates the handlified function signature convention:
  // raw pointer parameter |source_holder| in the function that allocates.
  // This is done intentionally to avoid ArrayConcat() builtin performance
  // degradation.
  virtual void CopyElements(Isolate* isolate, Tagged<JSObject> source_holder,
                            uint32_t source_start, ElementsKind source_kind,
                            DirectHandle<FixedArrayBase> destination,
                            uint32_t destination_start, int copy_size) = 0;

 private:
  V8_EXPORT_PRIVATE static ElementsAccessor** elements_accessors_;
};

V8_WARN_UNUSED_RESULT MaybeDirectHandle<Object>
ArrayConstructInitializeElements(Isolate* isolate, DirectHandle<JSArray> array,
                                 JavaScriptArguments* args);

// Called directly from CSA.
// {raw_context}: Context pointer.
// {raw_source}: JSArray pointer.
// {raw_destination}: JSTypedArray pointer.
void CopyFastNumberJSArrayElementsToTypedArray(Address raw_context,
                                               Address raw_source,
                                               Address raw_destination,
                                               uintptr_t length,
                                               uintptr_t offset);
// {raw_source}, {raw_destination}: JSTypedArray pointers.
void CopyTypedArrayElementsToTypedArray(Address raw_source,
                                        Address raw_destination,
                                        uintptr_t length, uintptr_t offset);
// {raw_source}, {raw_destination}: JSTypedArray pointers.
void CopyTypedArrayElementsSlice(Address raw_source, Address raw_destination,
                                 uintptr_t start, uintptr_t end);

}  // namespace internal
}  // namespace v8

#endif  // V8_OBJECTS_ELEMENTS_H_
