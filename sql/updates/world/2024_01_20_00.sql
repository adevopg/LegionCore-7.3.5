-- Fix loot chance for the following quest items

-- 2629 = Intrepid Strongbox Key
-- 3183 = Mangy Claw
-- 3618 = Gobbler's Head
-- 4503 = Witherbark Tusk
-- 4515 = Marez's Head
-- 4516 = Otto's Head
-- 4517 = Falconcrest's Head
-- 4522 = Witherbark Medicine Pouch
-- 23687 = Blacktalon's Claws
-- 28513 = Demonic Rune Stone (should be negative)
-- 29795 = Burning Legion Gate Key
-- 30158 = Morkh's Shattered Armor
-- 31347 = Bleeding Hollow Torch (drops when it should not, loot chance was all over with one negative and rest positive)
-- 52305 = Humming Electrogizard
-- 55234 = Dumpy Level
-- 55988 = Glowerglare's Beard
-- 56013 = Meaty Crawler Claw
-- 56083 = Fossilized Bone
-- 56087 = Marshy Crocolisk Hide
-- 56088 = Ironforge Ingot
-- 56089 = Horrorjaw's Hide
-- 58779 = Shell of Shadra
-- 60737 = Stabthistle Seed

DELETE FROM `creature_loot_template` WHERE `item` IN (2629,3183,3618,4503,4515,4516,4517,4522,23687,28513,29795,30158,31347,52305,55234,55988,56013,56083,56087,56088,56089,58779,60737);
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`, `shared`) VALUES
(1160, 2629, -100, 0, 0, 1, 1, 0),
(41429, 2629, -100, 0, 0, 1, 1, 0),

(1961, 3183, -100, 0, 0, 1, 1, 0),
(1259, 3618, -100, 0, 0, 1, 1, 0),

(2552, 4503, -40, 0, 0, 1, 1, 0),
(2553, 4503, -40, 0, 0, 1, 1, 0),
(2554, 4503, -40, 0, 0, 1, 1, 0),
(2555, 4503, -40, 0, 0, 1, 1, 0),
(2556, 4503, -40, 0, 0, 1, 1, 0),
(2557, 4503, -40, 0, 0, 1, 1, 0),
(2558, 4503, -40, 0, 0, 1, 1, 0),
(2605, 4503, -100, 0, 0, 1, 1, 0),
(51631, 4503, -40, 0, 0, 1, 1, 0),
(51633, 4503, -40, 0, 0, 1, 1, 0),

(2783, 4515, -100, 0, 0, 1, 1, 0),
(2599, 4516, -100, 0, 0, 1, 1, 0),
(2597, 4517, -100, 0, 0, 1, 1, 0),

(2555, 4522, -40, 0, 0, 1, 1, 0),
(51633, 4522, -40, 0, 0, 1, 1, 0),

(17057, 23687, -100, 0, 0, 1, 1, 0),

(16950, 28513, -60, 0, 0, 1, 1, 0),
(18975, 28513, -85, 0, 0, 1, 1, 0),
(18981, 28513, -60, 0, 0, 1, 1, 0),
(19190, 28513, -85, 0, 0, 1, 1, 0),
(19282, 28513, -25, 0, 0, 1, 1, 0),

(16946, 29795, -0.1699, 0, 0, 1, 1, 0),
(16947, 29795, -0.0916, 0, 0, 1, 1, 0),
(16954, 29795, -0.0267, 0, 0, 1, 1, 0),
(16960, 29795, -0.0187, 0, 0, 1, 1, 0),
(19298, 29795, -100, 0, 0, 1, 1, 0),

(16964, 30158, -100, 0, 0, 1, 1, 0),
(16907, 31347, -50, 0, 0, 1, 1, 0),
(37208, 52305, -25, 0, 0, 1, 1, 0),
(41145, 55234, -15, 0, 0, 1, 1, 0),

(41295, 56013, -50, 0, 0, 1, 1, 0),
(44116, 56013, -50, 0, 0, 1, 1, 0),

(41388, 56083, -100, 0, 0, 1, 3, 0),
(44226, 56083, -100, 0, 0, 1, 3, 0),

(41419, 56087, -100, 0, 0, 1, 1, 0),

(41390, 56088, -80, 0, 0, 1, 1, 0),
(41391, 56088, -80, 0, 0, 1, 1, 0),

(41420, 56089, -100, 0, 0, 1, 1, 0),
(42919, 58779, -100, 0, 0, 1, 1, 0),
(41151, 55988, -100, 0, 0, 1, 1, 0),

(44635, 60737, -2, 0, 0, 1, 1, 0),
(44638, 60737, -2, 0, 0, 1, 1, 0);

-- Fix "Young Murk Thresher" drop table (which will fix the "Thresher Oil" quest drop)

-- NOTE: This is for CREATURE, not ITEM!
DELETE FROM `creature_loot_template` WHERE `entry` IN (4388);
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`, `shared`) VALUES
(4388, 1529, 0.06, 0, 0, 1, 1, 0),
(4388, 1705, 0.03, 0, 0, 1, 1, 0),
(4388, 1725, 0.03, 0, 0, 1, 1, 0),
(4388, 2608, 10, 0, 0, 1, 1, 0),
(4388, 3864, 0.03, 0, 0, 1, 1, 0),
(4388, 4634, 0.1, 0, 0, 1, 1, 0),
(4388, 4636, 0.1, 0, 0, 1, 1, 0),
(4388, 5516, 22, 0, 0, 1, 1, 0),
(4388, 5637, 6, 0, 0, 1, 1, 0),
(4388, 6362, 15, 0, 0, 1, 2, 0),
(4388, 7084, 0.03, 0, 0, 1, 1, 0),
(4388, 7973, 45, 0, 0, 1, 1, 0),
(4388, 33126, -60, 0, 0, 1, 1, 0); -- Thresher Oil