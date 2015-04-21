#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#import <TargetConditionals.h>
#import <Availability.h>


void useMyHelperPlease();

/*!  ObjcAssociatedObjectHelpers.h -  ObjcAssociatedObjectHelpers    ┏(-_-)┛┗(-_-)┓┗(-_-)┛┏(-_-)┓

  @note All the macros have a @c_BLOCK suffix companion which takes a @c dispatch_block_t@c-type void block in the format @c void(^block)()
        for the getter, AND setter (if available). This allows additional code to be run in the accessors, similar to overriding an accessor.

	@warning    Since these are preprocessor macros, it's not possible to pass @c nil to any of these macros. Instead, pass an empty block; @c ^{}.
	@discussion In the context of the macro, the passed setter value, or the current associated value will be available as the symbol @c value !
              Its type will be appropriate to the context in which the macro was declared. 
              @c value is always declared with the @c __block attribute and so can be modified inside the block.
  @note       This is a little cumbersome since, *as far as I know*, there is no way to specify block parameter types in a macro and have the @c value variable passed explicitly into the block.
              If there is a way, [I'd love to here about it](mailto:joncrooke@gmail.com).
  @code

	    SYNTHESIZE_ASC_OBJ					(      readWriteObject,	setReadWriteObject    );  // Most basic form.  Only objects!
			SYNTHESIZE_ASC_OBJ_LAZY			(						lazyObject,	NSString.class				);
			SYNTHESIZE_ASC_OBJ_ASSIGN		(						 assignObj,	setAssignObj					);
			SYNTHESIZE_ASC_OBJ_LAZY_EXP	( nonDefaultLazyObject,	@"foo"								);
			SYNTHESIZE_ASC_PRIMITIVE		(            primitive,	setPrimitive, NSUI    );
			SYNTHESIZE_ASC_PRIMITIVE		(            structure,	setStructure, StructA	);
      SYNTHESIZE_ASC_CAST     		(               object,	setOject, cast        );  // When it's a block property.

*/

# /**.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:._.:*~*:.*/	pragma mark Quotation helper

#define __OBJC_ASC_QUOTE(x)                 #x
#define   OBJC_ASC_QUOTE(x) __OBJC_ASC_QUOTE(x)

@interface NSObject (AssociatedDictionary) @property (readonly) NSMutableDictionary *associatedDictionary; @end


# /**--/\/--/\/--/\/--/\/--/\/--/\/--/\/--/\/--/\/--/\/--/\/--/\/--\*/	pragma mark Assign (readwrite)

#define SYNTHESIZE_ASC_OBJ_ASSIGN(getter, setter) SYNTHESIZE_ASC_OBJ_ASSIGN_BLOCK(getter, setter, ^{}, ^{})

#define SYNTHESIZE_ASC_OBJ_ASSIGN_BLOCK(getter, setter, getterBlk, setterBlk)                         \
  static void* getter##Key = OBJC_ASC_QUOTE(getter);                                                  \
  - (void)setter:(id)__newValue {	__block id value = __newValue;  setterBlk();												\
    @synchronized(self) { objc_setAssociatedObject(self,getter##Key,value,OBJC_ASSOCIATION_ASSIGN); } \
  }                                                                                                   \
  - getter { __block id value = nil;                                                             \
    @synchronized(self) { value = objc_getAssociatedObject(self,getter##Key); };                      \
    return getterBlk(), value;                                                                        \
  }

#define SYNTHESIZE_ASC_OBJ_ONSET(getter, setter, onset) \
  static void* getter##Key = OBJC_ASC_QUOTE(getter);                                                      \
  - (void) setter:__newValue { objc_setAssociatedObject(self, getter##Key, __newValue,                    \
      [__newValue conformsToProtocol:@protocol(NSCopying)] ? OBJC_ASSOCIATION_COPY : OBJC_ASSOCIATION_RETAIN); \
    onset();                                                                                                     \
  }                                                                                                       \
  - getter { return objc_getAssociatedObject(self, getter##Key); }


# /** -<<O>>--<<O>>--<<O>>--<<O>>--<<O>>--<<O>>--<<O>>--<<O>>--<<O>>-*/	pragma mark Readwrite Object

/** Synthesize a getter and setter for a read/write object property. 
	@discussion If you would like to generate a read-only property with a private or protected setter then you can define this in another category, with the same name and type, but with a (readwrite) ualifier.  it can then be assigned a value (in init, etc.)
	@param getter Unquoted string that is the same as the property name declared in @interface.
	@param setter Unquoted string that is that will "set" your property, named in @interface.  ie. for property named "goal", "setGoal"
*/

#define SYNTHESIZE_ASC_OBJ(getter, setter) SYNTHESIZE_ASC_OBJ_BLOCK(getter, setter, ^{}, ^{})

#define SYNTHESIZE_ASC_OBJ_BLOCK(getter, setter, getterBlk, setterBlk)                                    \
  static void* getter##Key = OBJC_ASC_QUOTE(getter);                                                      \
  - (void) setter:__newValue { __block id value = __newValue; setterBlk();                             \
    @synchronized(self) { objc_setAssociatedObject(self, getter##Key, value,                              \
      [value conformsToProtocol:@protocol(NSCopying)] ? OBJC_ASSOCIATION_COPY : OBJC_ASSOCIATION_RETAIN); \
    }                                                                                                     \
  }                                                                                                       \
  - getter { __block id value = nil;                                                                 \
    @synchronized(self) { value = objc_getAssociatedObject(self, getter##Key); };                         \
    return getterBlk(), value;                                                                            \
  }

#/** .*O*.*O*.*O*.*O*.*O*.*O*.*O*.*O*.*O*.*O*.*O*.*O*.*O*.*O*.*O*.*O*.*/ pragma mark Lazy readonly object

/**
4. `SYNTHESIZE_ASC_OBJ_LAZY_EXP(getter, initExpression)` - Synthesize a read-only object that in initialized lazily, with the provided initialiser Expression. For example;

		SYNTHESIZE_ASC_OBJ_LAZY_EXP(nonDefaultLazyObject, [NSString stringWithFormat:@"foo"])	 
	Uses the expression `[NSString stringWithFormat:@"foo"]` to initialise the object. Note that `SYNTHESIZE_ASC_OBJ_LAZY` uses this macro with `[class.alloc init]`.
*/
#define SYNTHESIZE_ASC_OBJ_LAZY_EXP(getter, initExpression) \
  SYNTHESIZE_ASC_OBJ_LAZY_EXP_BLOCK(getter, initExpression, ^{})

#define SYNTHESIZE_ASC_OBJ_LAZY_EXP_BLOCK(getter, initExpression, block)            \
static void* getter##Key = OBJC_ASC_QUOTE(getter); 														  \
- getter {  __block id value = nil;																							\
  @synchronized(self) { 																																\
    value = objc_getAssociatedObject(self, getter##Key); 														\
    if (!value) { 																																		  \
      value = initExpression; 																													\
      objc_setAssociatedObject(self, getter##Key, value, OBJC_ASSOCIATION_RETAIN);  \
    } 																																									\
  } 																																		                \
  block(); 																																		          \
  return value; 																																		    \
}

/**
3. `SYNTHESIZE_ASC_OBJ_LAZY(getter, class)` - Synthesize a read-only object that in initialized lazily. The object's class must be provided so that an object can be initialized (with `alloc/init`) on first access.
*/
// Use default initialiser
#define SYNTHESIZE_ASC_OBJ_LAZY(getter, class) SYNTHESIZE_ASC_OBJ_LAZY_EXP_BLOCK(getter, [class new], ^{})

#define SYNTHESIZE_ASC_OBJ_LAZY_BLOCK(getter, class, block) \
  SYNTHESIZE_ASC_OBJ_LAZY_EXP_BLOCK(getter, [class new], block)


# /** -=*****=-.-=*****=-.-=*****=-.-=*****=-.-=*****=-.-=*****=-.*/		pragma mark Primitive

/**
2. `SYNTHESIZE_ASC_PRIMITIVE(getter, setter, type)` - Synthesize for any kind of primitive object. Any type supported by the `@encode()` operator is supported. So that *should* be everything…?
*/

#define SYNTHESIZE_ASC_PRIMITIVE_KVO(getter, setter, type) \
  SYNTHESIZE_ASC_PRIMITIVE_BLOCK_KVO(getter, setter, type, ^{}, ^{})

#define SYNTHESIZE_ASC_PRIMITIVE_BLOCK_KVO(getter, setter, type, getBlock, setBlock)   \
- (void)setter:(type)__newValue {  __block type value = __newValue; 												\
  setBlock();																																								\
  if (AZIsEqualToObject(@encode(type), &value, [self valueForKey:NSSTRINGIFY(getter)])) return;  /*  if (self.getter == value) return; \*/\
  [self willChangeValueForKey:NSSTRINGIFY(getter)]; \
    objc_setAssociatedObject(self, @selector(getter), 																			\
      [NSValue value:&value withObjCType:@encode(type)], OBJC_ASSOCIATION_RETAIN);					    \
  [self didChangeValueForKey:NSSTRINGIFY(getter)];  																				\
} 																																		                          \
- (type) getter {  __block type value; 																											\
  memset(&value, 0, sizeof(type)); 																															\
  @synchronized(self) { [objc_getAssociatedObject(self, _cmd) getValue:&value]; }		\
  getBlock();																																								\
  return value; 																																		            \
}



#define SYNTHESIZE_ASC_PRIMITIVE(getter, setter, type) \
  SYNTHESIZE_ASC_PRIMITIVE_BLOCK(getter, setter, type, ^{}, ^{})

#define SYNTHESIZE_ASC_PRIMITIVE_BLOCK(getter, setter, type, getterBlk, setterBlk)  \
static void* getter##Key = OBJC_ASC_QUOTE(getter);																			\
- (void)setter:(type)__newValue {  __block type value = __newValue; 												\
  setterBlk();																																								\
  @synchronized(self) {																																					\
    objc_setAssociatedObject(self, getter##Key, 																					  \
      [NSValue value:&value withObjCType:@encode(type)], OBJC_ASSOCIATION_RETAIN);					    \
  } 																																		                        \
} 																																		                          \
- (type) getter {  __block type value; 																											\
  memset(&value, 0, sizeof(type)); 																															\
  @synchronized(self) { [objc_getAssociatedObject(self, getter##Key) getValue:&value]; }		\
  getterBlk();																																								\
  return value; 																																		            \
}


#pragma mark - todo
// FIX DOCS
# /** -<<O>>--<<O>>--<<O>>--<<O>>--<<O>>--<<O>>--<<O>>--<<O>>--<<O>>-*/	pragma mark Readwrite Object

/** Synthesize a getter and setter for a read/write object property. 
	@discussion If you would like to generate a read-only property with a private or protected setter then you can define this in another category, with the same name and type, but with a (readwrite) ualifier.  it can then be assigned a value (in init, etc.)
	@param getter Unquoted string that is the same as the property name declared in @interface.
	@param setter Unquoted string that is that will "set" your property, named in @interface.  ie. for property named "goal", "setGoal"
*/

#define SYNTHESIZE_ASC_CAST(getter, setter, casting) \
  SYNTHESIZE_ASC_CAST_BLOCK(getter, setter, casting, ^{},^{}) \

#define SYNTHESIZE_ASC_CAST_BLOCK(getter, setter, casting, getBlock, setBlock) \
static void* getter##Key = OBJC_ASC_QUOTE(getter);                                                \
- (void)setter:(casting)__newValue { __block casting value = __newValue; 												\
  setBlock();										                                                                  \
  objc_AssociationPolicy policy =                                                                         \
  [__newValue conformsToProtocol:@protocol(NSCopying)] ? OBJC_ASSOCIATION_COPY : OBJC_ASSOCIATION_RETAIN; \
  @synchronized(self) { objc_setAssociatedObject(self, getter##Key, value, policy); }            \
}                                                                                                         \
- (casting) getter { __block id value = nil;                                                          \
  @synchronized(self) { value = objc_getAssociatedObject(self, getter##Key); };                       \
  getBlock();\
  return value;                                                                                           \
}



/**
 * \@synthesizeAssociation synthesizes a property for a class using associated
 * objects. This is primarily useful for adding properties to a class within
 * a category.
 *
 * PROPERTY must have been declared with \@property in the interface of the
 * specified class (or a category upon it), and must be of object type.
 */
#define synthesizeAssociationDefault(CLASS, PROPERTY, DEFAULT) \
	dynamic PROPERTY; \
	void *ext_uniqueKey_ ## CLASS ## _ ## PROPERTY = &ext_uniqueKey_ ## CLASS ## _ ## PROPERTY; \
	\
	__attribute__((constructor)) \
	static void ext_ ## CLASS ## _ ## PROPERTY ## _synthesize (void) { \
		Class cls = objc_getClass(# CLASS); \
		objc_property_t property = class_getProperty(cls, # PROPERTY); \
		NSCAssert(property, @"Could not find property %s on class %@", # PROPERTY, cls); \
		\
		ext_propertyAttributes *attributes = ext_copyPropertyAttributes(property); \
		if (!attributes) { return NSLog(@"*** Could not copy property attributes for %@.%s", cls, # PROPERTY); } \
		\
		NSCAssert(!attributes->weak, @"@synthesizeAssociation does not support weak properties (%@.%s)", cls, # PROPERTY); \
		\
		objc_AssociationPolicy policy = OBJC_ASSOCIATION_ASSIGN; \
		switch (attributes->memoryManagementPolicy) { \
			case ext_propertyMemoryManagementPolicyRetain: \
				policy = attributes->nonatomic ? OBJC_ASSOCIATION_RETAIN_NONATOMIC : OBJC_ASSOCIATION_RETAIN; break; \
			case ext_propertyMemoryManagementPolicyCopy: \
				policy = attributes->nonatomic ? OBJC_ASSOCIATION_COPY_NONATOMIC : OBJC_ASSOCIATION_COPY; break; \
			case ext_propertyMemoryManagementPolicyAssign: break; \
			default: NSCAssert(NO, @"Unrecognized property memory management policy %i", (int)attributes->memoryManagementPolicy); \
		} \
		id getter = ^(id self){ id x = objc_getAssociatedObject(self, ext_uniqueKey_ ## CLASS ## _ ## PROPERTY); if (!x) x = DEFAULT; return x; }; \
		id setter = ^(id self, id value){ objc_setAssociatedObject(self, ext_uniqueKey_ ## CLASS ## _ ## PROPERTY, value, policy); }; \
		if (!ext_addBlockMethod(cls, attributes->getter, getter, "@@:")) { \
			NSCAssert(NO, @"Could not add getter %s for property %@.%s", sel_getName(attributes->getter), cls, # PROPERTY); \
		} \
		if (!ext_addBlockMethod(cls, attributes->setter, setter, "v@:@")) { \
			NSCAssert(NO, @"Could not add setter %s for property %@.%s", sel_getName(attributes->setter), cls, # PROPERTY); \
		} \
		free(attributes); \
	}








/* #if      !__has_feature(objc_arc)   // Need Clang ARC #warning Associated object macros require Clang ARC to be enabled #endif 


	Created by Jon Crooke on 01/10/2012. - Copyright (c) 2012 Jonathan Crooke. All rights reserved.
	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

/** Platform minimum requirements (associated object availability) */
#if		 (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR) && __IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_4_0
#error Associated references available from iOS 4.0
#elif  TARGET_OS_MAC && !(TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR) && __MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_10_6
#error Associated references available from OS X 10.6
#endif
