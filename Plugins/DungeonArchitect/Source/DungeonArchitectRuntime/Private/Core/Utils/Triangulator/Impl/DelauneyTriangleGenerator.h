//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/Triangulator/DelauneyTriangulator.h"

class DUNGEONARCHITECTRUNTIME_API DelauneyTriangleGenerator : public DelauneyTriangulator {
protected:
    virtual void PerformTriangulation() override;

private:

};
