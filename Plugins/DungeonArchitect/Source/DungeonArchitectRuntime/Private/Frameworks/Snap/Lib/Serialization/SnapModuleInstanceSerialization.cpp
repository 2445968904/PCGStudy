//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Snap/Lib/Serialization/SnapModuleInstanceSerialization.h"


TSoftObjectPtr<UWorld> FSnapModuleInstanceSerializedData::GetThemedLevel(const FString& InThemeName) const {
	const TSoftObjectPtr<UWorld>* WorldPtr = ThemedLevels.Find(InThemeName);
	return WorldPtr ? *WorldPtr : nullptr;
}

