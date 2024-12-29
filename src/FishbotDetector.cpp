#include "ScriptMgr.h"
#include "Player.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "World.h"
#include "Config.h"
#include "Chat.h"
#include "ObjectAccessor.h"

#include <unordered_map>
#include <ctime> // time(), difftime()

// Зберігаємо інформацію про "останній каст" та лічильник підозр
struct FishInfo
{
    time_t lastCastTime;
    uint32 suspiciousCount;
};

class FishbotDetector : public PlayerScript
{
public:
    // Конструктор: читаємо налаштування з .conf (якщо підключені)
    FishbotDetector() : PlayerScript("FishbotDetector")
    {
        m_minInterval = sConfigMgr->GetIntDefault("FishbotDetector.MinInterval", 10);
        m_maxStrikes  = sConfigMgr->GetIntDefault("FishbotDetector.MaxStrikes", 5);

        TC_LOG_INFO("server.loading", "[FishbotDetector] MinInterval = %u, MaxStrikes = %u",
            m_minInterval, m_maxStrikes);
    }

    // Увага: сигнатура має збігатися з тією, що в PlayerScript:
    void OnSpellCastStart(Player* player, Spell* spell) override
    {
        if (!player || !spell)
            return;

        const SpellInfo* spellInfo = spell->GetSpellInfo();
        if (!spellInfo)
            return;

        // Змініть на реальний ID Fishing, якщо потрібно (у прикладі: 131476)
        static const uint32 FISHING_SPELL_ID = 7620;

        if (spellInfo->Id == FISHING_SPELL_ID)
        {
            uint64 guid = player->GetGUID();
            time_t now  = time(nullptr);

            FishInfo& info = m_fishData[guid];
            double diff = difftime(now, info.lastCastTime);
            info.lastCastTime = now;

            // Якщо різниця між двома кастами менше m_minInterval
            if (diff < m_minInterval)
            {
                info.suspiciousCount++;

                if (info.suspiciousCount >= m_maxStrikes)
                {
                    // Надсилаємо попередження в загальний чат
                    sWorld->SendServerMessage(SERVER_MSG_STRING,
                        "|cFFFF0000[AntiBot]|r Гравець " + std::string(player->GetName()) +
                        " підозріло часто кастує рибалку! (можливий fishbot)."
                    );

                    // При бажанні: player->KickPlayer();
                    // info.suspiciousCount = 0; // щоб не спамити безкінечно
                }
            }
            else
            {
                // Якщо інтервал нормальний - обнуляємо лічильник
                info.suspiciousCount = 0;
            }
        }
    }

private:
    static std::unordered_map<uint64, FishInfo> m_fishData;

    // Налаштування (читаються з .conf)
    uint32 m_minInterval;
    uint32 m_maxStrikes;
};

// Ініціалізація статичного контейнера
std::unordered_map<uint64, FishInfo> FishbotDetector::m_fishData;

void Addmod_fishbot_detectorScripts()
{
    new FishbotDetector();
}
