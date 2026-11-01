#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cstdint>
#include <array>
#include "inipp.hpp"

struct EventSetMetadata {
    std::string name;
    std::vector<std::string> events;
};

auto read_event_database() -> std::vector<EventSetMetadata> {
    std::ifstream file("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Cars Mater-National\\DataPC\\C\\Act\\EventInfo.dat", std::ios::in);
    inipp::Ini ini;
    ini.parse(file);

    std::vector<EventSetMetadata> event_sets{};
    for (const auto& general_param : ini.sections["General"].parameters) {
        if (general_param.first == "EventSet") {
            std::vector<std::string> events;
            for (const auto& event_set_param : ini.sections[general_param.second].parameters) {
                if (event_set_param.first == "Event") {
                    events.push_back(event_set_param.second);
                }
            }
            event_sets.emplace_back(general_param.second, std::move(events));
        }
    }
    return event_sets;
}

struct VehicleMetadata {
    std::string name;
    std::vector<std::string> paint_jobs;
    std::vector<std::string> wheel_sets;
};

auto read_vehicle_database() -> std::vector<VehicleMetadata> {
    std::ifstream file("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Cars Mater-National\\DataPC\\C\\Global\\Chars\\VehicleInfo.dat", std::ios::in);
    inipp::Ini ini;
    ini.parse(file);

    std::vector<VehicleMetadata> vehicles;

    for (const auto& vehicle_param : ini.sections["Vehicles"].parameters) {
        if (vehicle_param.first == "Vehicle") {
            VehicleMetadata meta{};
            for (const auto& vehicle_metadata_param : ini.sections[vehicle_param.second].parameters) {
                if (vehicle_metadata_param.first == "PaintJob") {
                    meta.paint_jobs.push_back(vehicle_metadata_param.second);
                }
                else if (vehicle_metadata_param.first == "WheelSet") {
                    meta.wheel_sets.push_back(vehicle_metadata_param.second);
                }
            }
            meta.name = vehicle_param.second;
            vehicles.push_back(std::move(meta));
        }
    }
    return vehicles;
}

auto read_story_database() -> std::vector<std::string> {
    std::ifstream file("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Cars Mater-National\\DataPC\\C\\Story\\StoryInfo.dat", std::ios::in);
    inipp::Ini ini;
    ini.parse(file);

    std::vector<std::string> story_events{};
    for (const auto& general_param : ini.sections["General"].parameters) {
        if (general_param.first == "StoryEvent") {
            story_events.push_back(general_param.second);
        }
    }
    return story_events;
}

struct PartGroupMetadata {
    std::string name;
    std::vector<std::string> parts;
};

auto read_scavenger_hunt_data() -> std::vector<PartGroupMetadata> {
    std::ifstream file("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Cars Mater-National\\DataPC\\C\\Act\\ScavengerHuntData.txt", std::ios::in);
    inipp::Ini ini;
    ini.parse(file);

    std::vector<PartGroupMetadata> part_groups{};
    for (const auto& item_group : ini.sections["ItemGroups"].parameters) {
        if (item_group.first == "ItemGroup") {
            std::vector<std::string> parts{};
            for (const auto& item_group_param : ini.sections[item_group.second].parameters) {
                if (item_group_param.first == "Item") {
                    parts.push_back(item_group_param.second);
                }
            }
            part_groups.emplace_back(item_group.second, std::move(parts));
        }
    }
    return part_groups;
}

#pragma pack(push, 1)
struct CarsProfile {
    char Name[20];
    std::uint32_t BoltBannersWon;
    std::uint32_t Points;
    float PercentComplete;
    std::uint32_t Unk1;
    std::uint32_t StartingBoltBanners;
    std::uint8_t StartingBonusPoints;
    std::uint32_t Unk2;
};

struct CarsBasicSettings {
    std::uint8_t DifficultyIndex;
    std::uint8_t Unk3;
    std::uint32_t Unk4;
    std::uint32_t Unk5;
    std::uint8_t EnableNonMusicAudio;
    std::uint8_t Unk6;
    std::uint8_t MusicVolume;
    std::uint8_t EffectsVolume;
    std::uint8_t UnkVolume; // DialogueRate?
    std::uint8_t DialogueVolume;
    std::uint8_t Unk7;
    std::uint8_t MapLayerVisible;
    std::uint8_t StatsLayerVisible;
    std::uint8_t Unk8;
    std::uint8_t Unk9;
    std::uint8_t IsKM;
    std::uint8_t IsKPH;
    std::uint8_t EnableAntiAliasing;
    std::uint8_t EnableFullscreenEffects;
    std::uint8_t EnforceFreeCameraSpeed;
    std::uint8_t UnkControlPlayer1;
    std::uint8_t UnkControlPlayer2;
    std::uint8_t Unk10;
    std::uint8_t Unk11;
};

struct CarsExtraSettings {
    std::uint8_t TimeOfDay; // IsNight?
    std::uint8_t EnableAutomaticRecentering;
    std::uint8_t BigAir;
    std::uint8_t Unk16;
    std::uint8_t TTCam;
    std::uint8_t Unk17;
    std::uint8_t ResolutionMode;
};

struct EventInfo {
    int Status;
    int Flags;
};

struct PaintJobRecord {
    std::uint32_t IsLocked;
};

struct WheelSet {
    std::uint32_t Unk1;
    std::uint32_t Unk2;
    std::uint32_t Unk3;
    std::uint32_t Unk4;
    std::uint32_t IsLocked;
};

struct StoryEvent {
    std::uint32_t Status;
    std::uint32_t BoltBannersEarned[3];
};

struct StoryManagerVehicleInfo {
    char VehicleName[40];
    char CharacterPaintJob[40];
    char CharacterWheelSet[40];
};

struct ExploreHub {
    char EventName[40];
    char AmbientVehicleGroup[40];
};

enum class RecordType : int {
    FastestTime = 1,
    SlowestTime,
    FastestRace,
    MostLaps,
    HighestScore,
    MostLaughs,
    BestSingleLap
};

struct Record {
    RecordType Type;
    std::uint32_t ProfileIndex0;
    float Value0;
    std::uint32_t ProfileIndex1;
    float Value1;
    std::uint32_t ProfileIndex2;
    float Value2;
};

struct BonusContentItem {
    std::uint32_t IsUnlocked;
};

static_assert(sizeof(CarsProfile) == 0x2D);
static_assert(sizeof(CarsBasicSettings) == 0x1E);
static_assert(sizeof(StoryManagerVehicleInfo) == 120);
#pragma pack(pop)

struct CarsSettings {
    CarsBasicSettings basic_settings;
    std::vector<std::uint8_t> default_playlist_1;
    std::vector<std::uint8_t> default_playlist_2;
    std::vector<std::uint8_t> default_playlist_3;
    CarsExtraSettings extra_settings;

    static CarsSettings parse(std::istream& stream) {
        CarsSettings settings{};
        stream.read(reinterpret_cast<char*>(&settings.basic_settings), sizeof(CarsBasicSettings));
        
        std::uint8_t song_count{};
        stream.read(reinterpret_cast<char*>(&song_count), 1);
        settings.default_playlist_1.resize(song_count);
        stream.read(reinterpret_cast<char*>(settings.default_playlist_1.data()), settings.default_playlist_1.size());
       
        stream.read(reinterpret_cast<char*>(&song_count), 1);
        settings.default_playlist_2.resize(song_count);
        stream.read(reinterpret_cast<char*>(settings.default_playlist_2.data()), settings.default_playlist_2.size());
        
        stream.read(reinterpret_cast<char*>(&song_count), 1);
        settings.default_playlist_3.resize(song_count);
        stream.read(reinterpret_cast<char*>(settings.default_playlist_3.data()), settings.default_playlist_3.size());

        stream.read(reinterpret_cast<char*>(&settings.extra_settings), sizeof(CarsExtraSettings));
        return settings;
    }

    void write(std::ostream& stream) const {
        
        stream.write(reinterpret_cast<const char*>(&basic_settings), sizeof(CarsBasicSettings));
        
        std::uint8_t unk_array_count = default_playlist_1.size();
        stream.write(reinterpret_cast<const char*>(&unk_array_count), 1);
        stream.write(reinterpret_cast<const char*>(default_playlist_1.data()), default_playlist_1.size());

        unk_array_count = default_playlist_2.size();
        stream.write(reinterpret_cast<const char*>(&unk_array_count), 1);
        stream.write(reinterpret_cast<const char*>(default_playlist_2.data()), default_playlist_2.size());

        unk_array_count = default_playlist_3.size();
        stream.write(reinterpret_cast<const char*>(&unk_array_count), 1);
        stream.write(reinterpret_cast<const char*>(default_playlist_3.data()), default_playlist_3.size());

        stream.write(reinterpret_cast<const char*>(&extra_settings), sizeof(CarsExtraSettings));
    }

    std::size_t len() const {
        return sizeof(CarsBasicSettings) + sizeof(CarsExtraSettings) + default_playlist_1.size() + default_playlist_2.size() + default_playlist_3.size();
    }
};

struct EventSet {
    std::vector<EventInfo> events;
};

struct EventDatabase {
    std::vector<EventSet> event_sets;

    static EventDatabase parse(std::istream& stream, const std::vector<EventSetMetadata>& event_set_metas) {
        std::vector<EventSet> event_sets;
        event_sets.reserve(event_set_metas.size());

        for (std::size_t i = 0; i < event_set_metas.size(); i++) {
            EventSet event_set{};
            event_set.events.reserve(event_set_metas[i].events.size());

            for (std::size_t j = 0; j < event_set_metas[i].events.size(); j++) {
                EventInfo event_info{};
                stream.read(reinterpret_cast<char*>(&event_info), sizeof(EventInfo));
                event_set.events.push_back(event_info);
            }
            event_sets.push_back(event_set);
        }
        return EventDatabase{ event_sets };
    }

    void write(std::ostream& stream) const {
        for (const auto& event_set : event_sets) {
            for (const auto& event_info : event_set.events) {
                stream.write(reinterpret_cast<const char*>(&event_info), sizeof(EventInfo));
            }
        }
    }
};

struct VehicleRecord {
    std::uint32_t is_locked;
    std::vector<PaintJobRecord> paint_job_records;
    std::vector<WheelSet> wheel_sets;
};

struct VehicleDatabase {
    std::vector<VehicleRecord> vehicle_records;

    static VehicleDatabase parse(std::istream& stream, const std::vector<VehicleMetadata>& vehicle_metas) {
        std::vector<VehicleRecord> vehicle_records;
        vehicle_records.reserve(vehicle_metas.size());

        for (std::size_t i = 0; i < vehicle_metas.size(); i++) {
            VehicleRecord vehicle{};
            vehicle.paint_job_records.resize(vehicle_metas[i].paint_jobs.size());
            vehicle.wheel_sets.resize(vehicle_metas[i].wheel_sets.size());

            stream.read(reinterpret_cast<char*>(&vehicle.is_locked), sizeof(std::uint32_t));
            stream.read(reinterpret_cast<char*>(vehicle.paint_job_records.data()), sizeof(PaintJobRecord) * vehicle.paint_job_records.size());
            stream.read(reinterpret_cast<char*>(vehicle.wheel_sets.data()), sizeof(WheelSet) * vehicle.wheel_sets.size());

            vehicle_records.push_back(vehicle);
        }
        return VehicleDatabase{ vehicle_records };
    }

    void write(std::ostream& stream) const {
        for (const auto& vehicle : vehicle_records) {
            stream.write(reinterpret_cast<const char*>(&vehicle.is_locked), sizeof(std::uint32_t));
            stream.write(reinterpret_cast<const char*>(vehicle.paint_job_records.data()), sizeof(PaintJobRecord) * vehicle.paint_job_records.size());
            stream.write(reinterpret_cast<const char*>(vehicle.wheel_sets.data()), sizeof(WheelSet) * vehicle.wheel_sets.size());
        }
    }
};

struct StoryManager {
    std::vector<StoryEvent> story_events;
    std::uint32_t current_vehicle_index;
    StoryManagerVehicleInfo normal_vehicle;
    StoryManagerVehicleInfo large_vehicle;
    std::array<std::uint32_t, 7> bumper_stickers_unlocked;
    std::array<ExploreHub, 4> explore_hubs;
    std::uint32_t current_explore_hub_index;
    std::uint32_t time_of_day;
    std::string current_story_event_name;
    std::uint32_t unk;
    std::uint32_t unk2;

    static StoryManager parse(std::istream& stream, const std::vector<std::string>& story_event_names) {
        std::vector<StoryEvent> story_events{};
        std::uint32_t current_vehicle_index{};
        StoryManagerVehicleInfo normal_vehicle{};
        StoryManagerVehicleInfo large_vehicle{};
        std::array<std::uint32_t, 7> bumper_stickers_unlocked{};
        std::array<ExploreHub, 4> explore_hubs{};
        std::uint32_t current_explore_hub_index{};
        std::uint32_t time_of_day{};
        char current_story_event_name[40]{};
        std::uint32_t unk{};
        std::uint32_t unk2{};

        story_events.resize(story_event_names.size());
        stream.read(reinterpret_cast<char*>(story_events.data()), sizeof(StoryEvent) * story_events.size());
        stream.read(reinterpret_cast<char*>(&current_vehicle_index), sizeof(std::uint32_t));
        stream.read(reinterpret_cast<char*>(&normal_vehicle), sizeof(StoryManagerVehicleInfo));
        stream.read(reinterpret_cast<char*>(&large_vehicle), sizeof(StoryManagerVehicleInfo));
        stream.read(reinterpret_cast<char*>(&bumper_stickers_unlocked), sizeof(std::uint32_t) * 7);
        stream.read(reinterpret_cast<char*>(&explore_hubs), sizeof(ExploreHub) * 4);
        stream.read(reinterpret_cast<char*>(&current_explore_hub_index), sizeof(std::uint32_t));
        stream.read(reinterpret_cast<char*>(&time_of_day), sizeof(std::uint32_t));
        stream.read(reinterpret_cast<char*>(&current_story_event_name), 40);
        stream.read(reinterpret_cast<char*>(&unk), sizeof(std::uint32_t));
        stream.read(reinterpret_cast<char*>(&unk2), sizeof(std::uint32_t));


        return StoryManager{ story_events, current_vehicle_index, normal_vehicle, large_vehicle, bumper_stickers_unlocked, explore_hubs, current_explore_hub_index, time_of_day, current_story_event_name, unk, unk2};
    }

    void write(std::ostream& stream) const {
        stream.write(reinterpret_cast<const char*>(story_events.data()), sizeof(StoryEvent) * story_events.size());
        stream.write(reinterpret_cast<const char*>(&current_vehicle_index), sizeof(std::uint32_t));
        stream.write(reinterpret_cast<const char*>(&normal_vehicle), sizeof(StoryManagerVehicleInfo));
        stream.write(reinterpret_cast<const char*>(&large_vehicle), sizeof(StoryManagerVehicleInfo));
        stream.write(reinterpret_cast<const char*>(&bumper_stickers_unlocked), sizeof(std::uint32_t) * 7);
        stream.write(reinterpret_cast<const char*>(&explore_hubs), sizeof(ExploreHub) * 4);
        stream.write(reinterpret_cast<const char*>(&current_explore_hub_index), sizeof(std::uint32_t));
        stream.write(reinterpret_cast<const char*>(&time_of_day), sizeof(std::uint32_t));
        stream.write(reinterpret_cast<const char*>(current_story_event_name.data()), current_story_event_name.size());
        for (std::size_t i = 0; i < 40 - current_story_event_name.size(); i++) {
            stream.put(0);
        }
        stream.write(reinterpret_cast<const char*>(&unk), sizeof(std::uint32_t));
        stream.write(reinterpret_cast<const char*>(&unk2), sizeof(std::uint32_t));
    }
};

struct PartGroup {
    std::uint32_t locked;
    std::uint32_t unk;
    std::vector<std::uint32_t> parts_found;
    std::uint32_t unk2;
};

struct ScavengerHuntManager {
    std::uint32_t metal_detector_enabled;
    std::int32_t part_group_index;
    std::vector<PartGroup> part_groups;
    std::uint32_t unk;

    static ScavengerHuntManager parse(std::istream& stream, const std::vector<PartGroupMetadata>& part_group_metas) {
        std::uint32_t metal_detector_enabled{};
        std::int32_t part_group_index{};
        std::vector<PartGroup> part_groups{};
        std::uint32_t unk{};

        stream.read(reinterpret_cast<char*>(&metal_detector_enabled), sizeof(std::uint32_t));
        stream.read(reinterpret_cast<char*>(&part_group_index), sizeof(std::int32_t));
        part_groups.reserve(part_group_metas.size());

        for (std::size_t i = 0; i < part_group_metas.size(); i++) {
            PartGroup part_group{};
            part_group.parts_found.resize(part_group_metas[i].parts.size());
            stream.read(reinterpret_cast<char*>(&part_group.locked), sizeof(std::uint32_t));
            stream.read(reinterpret_cast<char*>(&part_group.unk), sizeof(std::uint32_t));

            stream.read(reinterpret_cast<char*>(part_group.parts_found.data()), sizeof(std::uint32_t) * part_group.parts_found.size());
            stream.read(reinterpret_cast<char*>(&part_group.unk2), sizeof(std::uint32_t));

            part_groups.push_back(part_group);
        }
        stream.read(reinterpret_cast<char*>(&unk), sizeof(std::uint32_t));
        return ScavengerHuntManager{ metal_detector_enabled, part_group_index, part_groups, unk };
    }

    void write(std::ostream& stream) const {
        stream.write(reinterpret_cast<const char*>(&metal_detector_enabled), sizeof(std::uint32_t));
        stream.write(reinterpret_cast<const char*>(&part_group_index), sizeof(std::int32_t));
        for (const auto& part_group : part_groups) {
            stream.write(reinterpret_cast<const char*>(&part_group.locked), sizeof(std::uint32_t));
            stream.write(reinterpret_cast<const char*>(&part_group.unk), sizeof(std::uint32_t));
            stream.write(reinterpret_cast<const char*>(part_group.parts_found.data()), sizeof(std::uint32_t) * part_group.parts_found.size());
            stream.write(reinterpret_cast<const char*>(&part_group.unk2), sizeof(std::uint32_t));
        }
        stream.write(reinterpret_cast<const char*>(&unk), sizeof(std::uint32_t));
    }
};

struct RecordSet {
    std::string name;
    std::vector<Record> records;
    static RecordSet parse(std::istream& stream) {
        char name[64]{};
        stream.read(name, sizeof(name));
        std::uint32_t record_count = 0;
        stream.read(reinterpret_cast<char*>(&record_count), sizeof(std::uint32_t));
        std::vector<Record> records(record_count);
        stream.read(reinterpret_cast<char*>(records.data()), sizeof(Record) * records.size());
        return RecordSet { name, records };
    }   
};

struct RecordLibrary {
    std::uint32_t time_stamp; // stupid format, non-standard
    std::vector<std::string> profile_names;
    std::vector<RecordSet> record_sets;

    static RecordLibrary parse(std::istream& stream, int record_set_count) {
        std::uint32_t time_stamp = 0;
        stream.read(reinterpret_cast<char*>(&time_stamp), sizeof(std::uint32_t));

        std::uint32_t profile_name_count = 0;
        stream.read(reinterpret_cast<char*>(&profile_name_count), sizeof(std::uint32_t));

        char temp[32]{};
        std::vector<std::string> profile_names{};
        profile_names.reserve(profile_name_count);
        
        for (auto i = 0; i < profile_name_count; i++) {
            stream.read(reinterpret_cast<char*>(&temp), sizeof(temp));
            profile_names.push_back(temp);
        }

        std::vector<RecordSet> record_sets{};
        record_sets.reserve(record_set_count);

        for (auto i = 0; i < record_set_count; i++) {
            record_sets.push_back(RecordSet::parse(stream));
        }

        return RecordLibrary { time_stamp, profile_names, record_sets };
    }
};

struct BonusContentManager {
    std::vector<BonusContentItem> bonus_content_items;
    static BonusContentManager parse(std::istream& stream, int bonus_content_item_count) {
        std::vector<BonusContentItem> bonus_content_items(bonus_content_item_count);
        bonus_content_items.reserve(bonus_content_item_count);
        stream.read(reinterpret_cast<char*>(bonus_content_items.data()), bonus_content_items.size() * sizeof(BonusContentItem));
        return BonusContentManager{ bonus_content_items };
    }
};

struct SaveFile {
    CarsProfile profile;
    CarsSettings settings;
    EventDatabase event_database;
    VehicleDatabase vehicle_database;
    StoryManager story_manager;
    ScavengerHuntManager scavenger_hunt_manager;
    RecordLibrary personal_records;
    BonusContentManager bonus_content_manager;
    // PickupManager pickup_manager;
    // BoltManager bolt_manager;

    static SaveFile parse(std::istream& stream, const std::vector<EventSetMetadata>& event_set_metas, const std::vector<VehicleMetadata>& vehicle_metas, const std::vector<std::string>& story_event_names, const std::vector<PartGroupMetadata>& part_group_metas) {
        CarsProfile profile{};
        stream.read(reinterpret_cast<char*>(&profile), sizeof(CarsProfile));
        CarsSettings settings = CarsSettings::parse(stream);
        EventDatabase event_database = EventDatabase::parse(stream, event_set_metas);
        VehicleDatabase vehicle_database = VehicleDatabase::parse(stream, vehicle_metas);
        StoryManager story_manager = StoryManager::parse(stream, story_event_names);
        ScavengerHuntManager scavenger_hunt_manager = ScavengerHuntManager::parse(stream, part_group_metas);
        RecordLibrary personal_records = RecordLibrary::parse(stream, 59);
        BonusContentManager bonus_content_manager = BonusContentManager::parse(stream, 89);
        return SaveFile{ profile, settings, event_database, vehicle_database, story_manager, scavenger_hunt_manager, personal_records, bonus_content_manager};
    }
};

auto main() -> int
{
    auto event_info = read_event_database();
    auto vehicle_info = read_vehicle_database();
    auto story_info = read_story_database();
    auto scavenger_hunt_data = read_scavenger_hunt_data();
    std::ifstream save("C:\\Users\\risha\\OneDrive\\Documents\\THQ\\Cars2\\PSLOT0.dat", std::ios::in | std::ios::binary);
    SaveFile file = SaveFile::parse(save, event_info, vehicle_info, story_info, scavenger_hunt_data);
    std::println(std::cout, "{}", (uint64_t)save.tellg());
    return 0;
}
