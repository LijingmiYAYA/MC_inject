#pragma once
#include "HookA.h"
#include "mc.h"
#include "pch.h"

extern GameXuid xuid_addr;
extern GameXuid* pXuid;

class Gxuid
{
public:

    bool IsReadableMemory(void* ptr, SIZE_T size) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0)
            return false;

        bool isCommited = (mbi.State == MEM_COMMIT);
        bool isReadable = (mbi.Protect & PAGE_READONLY) ||
            (mbi.Protect & PAGE_READWRITE) ||
            (mbi.Protect & PAGE_WRITECOPY) ||
            (mbi.Protect & PAGE_EXECUTE_READ) ||
            (mbi.Protect & PAGE_EXECUTE_READWRITE) ||
            (mbi.Protect & PAGE_EXECUTE_WRITECOPY);

        bool notGuard = !(mbi.Protect & PAGE_GUARD);
        bool notNoAccess = !(mbi.Protect & PAGE_NOACCESS);

        return isCommited && isReadable && notGuard && notNoAccess && mbi.RegionSize >= size;
    }

    void init(GameXuid a) {

        this->aGameXuid = a;
        this->x = &a;

    }

    void init2(GameXuid* a) {

        this->x = a;

    }

    std::string GetName() {

        GameXuid* x1 = this->x;

        std::string name;

        if (x1->name_size > 9)
        {
            if (IsReadableMemory((void*)x1->name_pointer, 80)) {
                const char* name_ptr = reinterpret_cast<const char*>(x1->name_pointer);
                name.assign(name_ptr, x1->name_size);
            }
        }
        else
        {
            name.assign(reinterpret_cast<const char*>(&x1->name_pointer), x1->name_size);
        }

        return name;

    };

    std::string GetXuid() {

        GameXuid* x1 = this->x;

        std::string xuid;

        if (x1->xuid_size > 9)
        {
            if (IsReadableMemory((void*)x1->xuid, 80)) {
                const char* xuid_ptr = reinterpret_cast<const char*>(x1->xuid);
                xuid.assign(xuid_ptr, x1->xuid_size);
            }
        }
        else
        {
            xuid.assign(reinterpret_cast<const char*>(&x1->xuid), x1->xuid_size);
        }

        return xuid;

    };

    std::string NameFindXuid(std::string cdname) {

        GameXuid* x1 = this->x;

        for (size_t i = 0; i < 20; i++)
        {
            x1 = x1->next;

            std::string name;

            if (x1->name_size > 9)
            {
                if (IsReadableMemory((void*)x1->name_pointer, 80)) {
                    const char* name_ptr = reinterpret_cast<const char*>(x1->name_pointer);
                    name.assign(name_ptr, x1->name_size);
                }
            }
            else
            {
                name.assign(reinterpret_cast<const char*>(&x1->name_pointer), x1->name_size);
            }

            if (name == cdname)
            {
                std::string xuid;

                if (x1->xuid_size > 9)
                {
                    if (IsReadableMemory((void*)x->xuid, 80)) {
                        const char* xuid_ptr = reinterpret_cast<const char*>(x1->xuid);
                        xuid.assign(xuid_ptr, x1->xuid_size);
                    }
                }
                else
                {
                    xuid.assign(reinterpret_cast<const char*>(&x1->xuid), x1->xuid_size);
                }

                return xuid;

            }



        }


        return "";
    }

    std::string XuidFindName(std::string xXuid) {

        GameXuid* x1 = this->x;

        for (size_t i = 0; i < 20; i++)
        {
            x1 = x1->next;

            std::string xuid;

            if (x1->xuid_size > 9)
            {
                if (IsReadableMemory((void*)x1->name_pointer, 80)) {
                    const char* xuid_ptr = reinterpret_cast<const char*>(x1->xuid);
                    xuid.assign(xuid_ptr, x1->xuid_size);
                }
            }
            else
            {
                xuid.assign(reinterpret_cast<const char*>(&x1->xuid), x1->xuid_size);
            }

            if (xuid == xXuid)
            {
                std::string name;

                if (x1->name_size > 9)
                {
                    if (IsReadableMemory((void*)x1->name_pointer, 80)) {
                        const char* name_ptr = reinterpret_cast<const char*>(x1->name_pointer);
                        name.assign(name_ptr, x1->name_size);
                    }
                }
                else
                {
                    name.assign(reinterpret_cast<const char*>(&x1->name_pointer), x1->name_size);
                }

                return name;

            }



        }

        return "";
    }


private:
    GameXuid aGameXuid;
    Xuid bXuid;
    GameXuid* x;


};
