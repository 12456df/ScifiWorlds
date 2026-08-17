#include "Mass/SmokeTest/SWMassSmokeTestTrait.h"

#include "Mass/SmokeTest/SWMassSmokeTestFragments.h"
#include "MassEntityTemplateRegistry.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMassSmokeTestTrait)

void USWMassSmokeTestTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddFragment<FSWMassSmokeTestFragment>();
	BuildContext.AddTag<FSWMassSmokeTestTag>();
}
