#pragma once
#include <cstdint>
#include <vector>
#include <Windows.h>

namespace Weapons {

    // All weapon hashes
    static const std::vector<uint32_t> AllWeapons = {
        // Melee
        0x92A27487, // Knife
        0x958A4A8F, // Nightstick
        0xF9E6AA4B, // Hammer
        0x84BD7BFD, // Bat
        0x440E4788, // Crowbar
        0xDFE37640, // Golf Club
        0x5FC3C11,  // Knuckle Duster
        0xD8DF3C3C, // Machete
        0x99B507EA, // Switchblade
        0xDD5DF8D9, // Hatchet
        0x678B81B1, // Bottle
        0xFB3A263D, // Flashlight

        // Pistols
        0x1B06D571, // Pistol
        0xBFE256D4, // Pistol MK2
        0x57A89D2,  // Combat Pistol
        0x22D8FE39, // AP Pistol
        0x99AEEB3B, // Stun Gun
        0x3656C8C1, // Flare Gun
        0xDC4DB296, // Marksman Pistol
        0x97EA20B8, // Heavy Pistol
        0xCB96392,  // Vintage Pistol
        0x88374054, // SNS Pistol MK2
        0x5EF9FEC4, // Double Action Revolver
        0x969C3D67, // Navy Revolver
        0xC1B3C3D1, // Perico Pistol
        0x12E82D3D, // Ceramic Pistol

        // SMGs
        0x78A97CD0, // SMG
        0xA284510B, // SMG MK2
        0x9D1F17E6, // Assault SMG
        0xBD248B55, // Combat PDW
        0x7F229F94, // Machine Pistol
        0xDB1AA450, // Mini SMG
        0x44AE7910, // Tactical SMG

        // Shotguns
        0x1D073A89, // Pump Shotgun
        0x555AF99A, // Pump Shotgun MK2
        0x593FE1A7, // Sawed-Off Shotgun
        0x60E27978, // Assault Shotgun
        0xFBAB5776, // Musket
        0xEF951FBB, // Heavy Shotgun
        0x3AABBBAA, // Bullpup Shotgun MK2

        // Assault Rifles
        0xBFEFFF6D, // Assault Rifle
        0x394F415C, // Assault Rifle MK2
        0x83BF0278, // Carbine Rifle
        0xFAD1F1C9, // Carbine Rifle MK2
        0xAFBD60D6, // Advanced Rifle
        0x1E016595, // Special Carbine
        0x14E56347, // Special Carbine MK2
        0x9D07F764, // Bullpup Rifle
        0x84D6FAFD, // Bullpup Rifle MK2
        0xC78D71B4, // Compact Rifle
        0x24B17070, // Military Rifle
        0xC0A3098D, // Heavy Rifle

        // LMGs
        0x7FD62962, // MG
        0xDBBD7280, // Combat MG
        0x7F7497E5, // Combat MG MK2
        0x63ABADE7, // Gusenberg Sweeper

        // Sniper Rifles
        0x05FC3C11, // Sniper Rifle
        0xC734385A, // Heavy Sniper
        0x6E7DDDEC, // Marksman Rifle
        0xDFE037A9, // Marksman Rifle MK2

        // Heavy
        0xB1CA77B1, // RPG
        0x4DD2DC56, // Minigun
        0x6D544C99, // Homing Launcher
        0x781FE4A,  // Grenade Launcher

        // Throwables
        0x93E220BD, // Grenade
        0xAB564B93, // BZ Gas
        0xBA45E8B8, // Smoke Grenade
        0xFDBC8A50, // Flare
        0xA8DF15F6, // Molotov
        0x2C3731D9, // Proximity Mine
        0x43A5B996, // Pipe Bomb
        0x787F0BB,  // Tear Gas
        0x34A67B97, // Petrol Can
    };
}