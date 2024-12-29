#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "Player.h"
#include "World.h"
#include "Config.h"
#include <unordered_map>
#include <ctime>

// Зберігаємо дані про останню риболовлю гравця
struct FishInfo
{
    time_t lastCastTime;
    uint32 suspiciousCount;
};

class FishbotDetector : public PlayerScript
{
public:
    FishbotDetector() : PlayerScript("FishbotDetector")
    {
        // Зчитуємо з .conf
        m_minInterval = sConfigMgr->GetIntDefault("FishbotDetector.MinInterval", 10);
        m_maxStrikes  = sConfigMgr->GetIntDefault("FishbotDetector.MaxStrikes", 5);

        TC_LOG_INFO("server.loading",
            "[FishbotDetector] MinInterval = %u, MaxStrikes = %u",
            m_minInterval, m_maxStrikes
        );
    }

    // Подія "OnSpellCast" з master-ґілки AzerothCore:
    //   virtual void OnSpellCast(Player* player, Spell* spell);
    void OnSpellCast(Player* player, Spell* spell) override
    {
        if (!player || !spell)
            return;

        const SpellInfo* spellInfo = spell->GetSpellInfo();
        if (!spellInfo)
            return;

        // Змініть цей ID Fishing, якщо у вас інший
        static const uint32 FISHING_SPELL_ID = 7620;

        if (spellInfo->Id == FISHING_SPELL_ID)
        {
            uint64 guid = player->GetGUID();
            time_t now  = time(nullptr);

            FishInfo& info = m_fishData[guid];
            double diff = difftime(now, info.lastCastTime);
            info.lastCastTime = now;

            if (diff < m_minInterval)
            {
                info.suspiciousCount++;

                if (info.suspiciousCount >= m_maxStrikes)
                {
                    // Повідомлення у світовий чат
                    sWorld->SendServerMessage(SERVER_MSG_STRING,
                        "|cFFFF0000[AntiBot]|r " + std::string(player->GetName()) +
                        " підозріло часто закидає вудку! (fishbot?)"
                    );

                    // Можна також:
                    // player->KickPlayer();
                    // info.suspiciousCount = 0; // щоб не спамити, якщо залишати гравця в грі
                }
            }
            else
            {
                // Якщо інтервал нормальний, скидаємо лічильник
                info.suspiciousCount = 0;
            }
        }
    }

private:
    static std::unordered_map<uint64, FishInfo> m_fishData;
    uint32 m_minInterval;
    uint32 m_maxStrikes;
};

// Ініціалізація статичної змінної
std::unordered_map<uint64, FishInfo> FishbotDetector::m_fishData;

void Addmod_fishbot_detectorScripts()
{
    new FishbotDetector();
}
