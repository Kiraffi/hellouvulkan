
#include "gameentity.h"

bool findEntityType(std::string_view name, EntityType &outType)
{
    for(const auto entityName : nameStrings)
    {
        if(name == entityName.name)
        {
            outType = entityName.type;
            return true;
        }
    }
    return false;
}

bool writeJson(const GameEntity &entity)
{

}