DROP DATABASE IF EXISTS plant;
CREATE DATABASE plant;
USE plant;

-- ============================================================
-- USERS TABLE — Real authentication
-- ============================================================
CREATE TABLE users (
    user_id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,
    display_name VARCHAR(100),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    last_login DATETIME
);

-- ============================================================
-- SESSIONS TABLE — Server-side session management
-- ============================================================
CREATE TABLE sessions (
    session_id VARCHAR(64) PRIMARY KEY,
    user_id INT NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    expires_at DATETIME NOT NULL,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- ============================================================
-- PLANTS TABLE — Enhanced with full plant info
-- ============================================================
CREATE TABLE plants (
    plant_id INT AUTO_INCREMENT PRIMARY KEY,
    plant_name VARCHAR(100) UNIQUE NOT NULL,
    plant_type VARCHAR(50),
    soil_type VARCHAR(50),
    scientific_name VARCHAR(150) NOT NULL,
    water_amount_ml INT NOT NULL,
    water_interval_hours INT NOT NULL DEFAULT 12,
    lifespan_hours INT NOT NULL DEFAULT 18,
    sunlight VARCHAR(50) DEFAULT 'Partial Sun',
    difficulty VARCHAR(20) DEFAULT 'Medium',
    best_season VARCHAR(50) DEFAULT 'Year-round',
    benefits TEXT,
    description TEXT
);

-- ============================================================
-- MY GARDEN TABLE — with favorite support and user binding
-- ============================================================
CREATE TABLE my_garden (
    garden_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    plant_name VARCHAR(100) NOT NULL,
    planted_on DATETIME DEFAULT CURRENT_TIMESTAMP,
    last_watered DATETIME DEFAULT CURRENT_TIMESTAMP,
    died_on DATETIME NULL,
    status VARCHAR(20) DEFAULT 'ALIVE',
    is_favorite TINYINT DEFAULT 0,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- ============================================================
-- WATER HISTORY — Logs every watering event
-- ============================================================
CREATE TABLE water_history (
    history_id INT AUTO_INCREMENT PRIMARY KEY,
    garden_id INT NOT NULL,
    plant_name VARCHAR(100) NOT NULL,
    watered_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (garden_id) REFERENCES my_garden(garden_id) ON DELETE CASCADE
);

-- ============================================================
-- PLANT NOTES — User notes on garden plants
-- ============================================================
CREATE TABLE plant_notes (
    note_id INT AUTO_INCREMENT PRIMARY KEY,
    garden_id INT NOT NULL,
    note_text TEXT NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (garden_id) REFERENCES my_garden(garden_id) ON DELETE CASCADE
);

-- ============================================================
-- ACTIVITY LOG — Persistent activity tracking per user
-- ============================================================
CREATE TABLE activity_log (
    log_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NULL,
    action VARCHAR(50) NOT NULL,
    details VARCHAR(255),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE SET NULL
);

-- ============================================================
-- DEFAULT ADMIN USER (password: admin123)
-- ============================================================
INSERT INTO users (username, password, display_name) VALUES ('admin', 'admin123', 'Administrator');

-- ============================================================
-- ALL 121 PLANTS — with full info
-- ============================================================
INSERT INTO plants (plant_name, plant_type, soil_type, scientific_name, water_amount_ml, water_interval_hours, lifespan_hours, sunlight, difficulty, best_season, benefits, description) VALUES
('African Daisy','Flowering','Loamy','Osteospermum',40,12,18,'Full Sun','Easy','Spring','Brightens gardens with vibrant daisy-like blooms; attracts pollinators.','Colorful daisy native to South Africa. Blooms in cool weather and prefers well-drained soil. Deadhead regularly for continuous flowering.'),
('Agave','Succulent','Sandy','Agave americana',30,12,18,'Full Sun','Easy','Spring','Drought-tolerant landscaping; fibers used for rope and textiles.','Dramatic rosette-shaped succulent that stores water in thick leaves. Extremely low maintenance. Can live for decades before flowering once.'),
('Ageratum','Flowering','Loamy','Ageratum houstonianum',35,12,18,'Full Sun','Easy','Spring','Excellent border plant; attracts butterflies and bees.','Compact fluffy flower clusters in blue, pink, or white. Great for edging and containers. Prefers warm weather and consistent moisture.'),
('Air Plant','Epiphyte','No Soil','Tillandsia',20,12,18,'Indirect Light','Easy','Year-round','Natural air purifier; unique no-soil decoration.','Soilless plant that absorbs moisture through its leaves. Mist 2-3 times weekly. Perfect for terrariums and creative displays.'),
('Almond Plant','Tree','Loamy','Prunus dulcis',70,12,18,'Full Sun','Hard','Winter','Nutritious almonds rich in protein, vitamin E, and healthy fats.','Deciduous tree producing valuable nuts. Needs cold winters for dormancy. Takes 3-5 years to bear fruit. Requires cross-pollination.'),
('Aloe Vera','Succulent','Sandy','Aloe barbadensis miller',30,12,18,'Indirect Light','Easy','Year-round','Gel soothes burns, moisturizes skin; purifies indoor air.','Medicinal succulent with thick gel-filled leaves. Let soil dry between waterings. Easy to propagate from offsets. Keep near a sunny window.'),
('Amaryllis','Flowering','Loamy','Hippeastrum',45,12,18,'Indirect Light','Medium','Winter','Stunning large blooms for indoor winter color; easy to force bloom.','Produces large trumpet-shaped flowers on tall stalks. Popular holiday gift plant. Store bulbs dry in fall to re-bloom next season.'),
('Anthurium','Flowering','Peaty','Anthurium andraeanum',45,12,18,'Indirect Light','Medium','Year-round','Excellent air purifier; long-lasting waxy flowers for decor.','Tropical plant with glossy heart-shaped flowers lasting weeks. Prefers humidity and indirect light. Wipe leaves to keep them shiny.'),
('Apple Sapling','Fruit','Loamy','Malus domestica',80,12,18,'Full Sun','Hard','Winter','Fresh nutritious fruit; beautiful spring blossoms attract bees.','Deciduous fruit tree needing cold winters. Most varieties need cross-pollination. Prune annually for best fruit production. Takes 3-5 years to fruit.'),
('Areca Palm','Indoor','Loamy','Dypsis lutescens',65,12,18,'Indirect Light','Medium','Spring','Top NASA-rated air purifier; natural humidifier for dry rooms.','Elegant feathery palm perfect for bright rooms. Grows 6-7 feet indoors. Water when top soil feels dry. Sensitive to fluoride in tap water.'),
('Ashwagandha','Medicinal','Sandy','Withania somnifera',35,12,18,'Full Sun','Medium','Monsoon','Adaptogenic herb that reduces stress and boosts immunity.','Ancient Ayurvedic medicinal herb. Roots are the primary medicinal part. Grows as a small shrub. Harvest roots after 150-180 days.'),
('Asparagus Fern','Indoor','Peaty','Asparagus densiflorus',40,12,18,'Indirect Light','Easy','Spring','Feathery foliage adds texture to arrangements; air purifying.','Not a true fern but a member of the lily family. Produces tiny white flowers and red berries. Loves humidity and indirect light.'),
('Aster','Flowering','Loamy','Asteraceae',40,12,18,'Full Sun','Easy','Monsoon','Late-season color for gardens; attracts butterflies and bees.','Star-shaped flowers that bloom in late summer and fall when other flowers fade. Available in purple, pink, blue, and white.'),
('Avocado Plant','Fruit','Loamy','Persea americana',85,12,18,'Full Sun','Hard','Spring','Nutrient-dense fruit with healthy fats, potassium, and fiber.','Tropical tree that can be started from a pit. Indoor plants rarely fruit but make attractive houseplants. Needs consistent warmth and moisture.'),
('Azalea','Flowering','Acidic','Rhododendron',50,12,18,'Partial Shade','Medium','Spring','Spectacular spring blooms; excellent foundation shrub.','Acid-loving shrub with masses of colorful spring flowers. Mulch with pine needles to maintain soil acidity. Avoid alkaline water.'),
('Bamboo','Indoor','Loamy','Bambusoideae',60,12,18,'Indirect Light','Easy','Spring','Lucky bamboo brings positive energy; natural privacy screen.','Fast-growing grass family member. Lucky bamboo grows in water or soil. Real bamboo needs containment to prevent spreading.'),
('Banana Plant','Fruit','Loamy','Musa',90,12,18,'Full Sun','Medium','Monsoon','Nutritious fruit rich in potassium; large leaves provide tropical shade.','Tropical plant that grows from a corm, not a tree. Needs heat, humidity, and heavy feeding. Fruit in 9-12 months in warm climates.'),
('Basil','Herb','Loamy','Ocimum basilicum',35,12,18,'Full Sun','Easy','Summer','Essential culinary herb; repels mosquitoes; aids digestion.','Aromatic herb essential in Italian and Thai cuisine. Pinch flower buds to encourage bushy growth. Harvest leaves from the top down.'),
('Bay Leaf','Herb','Loamy','Laurus nobilis',40,12,18,'Full Sun','Medium','Spring','Culinary seasoning; aids digestion; natural insect repellent.','Evergreen tree whose leaves flavor soups and stews. Can be grown as a container plant. Dry leaves before using for best flavor.'),
('Begonia','Flowering','Peaty','Begonia',40,12,18,'Partial Shade','Medium','Spring','Beautiful foliage and flowers for shaded areas; easy container plant.','Diverse genus with thousands of varieties. Some grown for flowers, others for decorative leaves. Avoid overwatering and direct sun.'),
('Bell Pepper','Vegetable','Loamy','Capsicum annuum',50,12,18,'Full Sun','Medium','Summer','Rich in vitamin C; versatile in cooking; colorful garden addition.','Warm-season vegetable producing green, red, yellow, or orange fruits. Start indoors 8 weeks before last frost. Needs consistent moisture.'),
('Betel Leaf','Medicinal','Loamy','Piper betle',45,12,18,'Partial Shade','Medium','Monsoon','Aids digestion; antiseptic properties; traditional medicinal uses.','Heart-shaped glossy leaves used in traditional medicine and cultural practices. Climbing vine that needs support. Prefers warm, humid conditions.'),
('Bird of Paradise','Flowering','Loamy','Strelitzia reginae',60,12,18,'Full Sun','Medium','Spring','Stunning tropical flower; excellent cut flower lasting weeks.','Exotic flower resembling a tropical bird. Takes 3-5 years to bloom from seed. Dramatic houseplant even without flowers.'),
('Black Pepper','Spice','Loamy','Piper nigrum',45,12,18,'Partial Shade','Hard','Monsoon','King of spices; aids digestion; rich in antioxidants.','Tropical climbing vine producing peppercorns. Needs warm humid conditions and a support structure. Takes 3-4 years to produce fruit.'),
('Blueberry Plant','Fruit','Acidic','Vaccinium',60,12,18,'Full Sun','Medium','Spring','Superfood rich in antioxidants; beautiful fall foliage color.','Acid-loving fruit bush producing sweet berries. Needs acidic soil pH 4.5-5.5. Plant two varieties for better cross-pollination.'),
('Bougainvillea','Flowering','Sandy','Bougainvillea',50,12,18,'Full Sun','Easy','Spring','Vibrant cascading color for walls and arbors; drought-tolerant.','Tropical vine with papery bracts in magenta, purple, orange, or white. Thrives in heat with minimal water. Stress encourages blooming.'),
('Brahmi','Medicinal','Clay','Bacopa monnieri',30,12,18,'Partial Shade','Easy','Monsoon','Enhances memory and cognitive function; reduces anxiety naturally.','Creeping herb used in Ayurveda for brain health. Grows well near water or in moist soil. Small white flowers appear year-round.'),
('Broccoli Plant','Vegetable','Loamy','Brassica oleracea',55,12,18,'Full Sun','Medium','Winter','Packed with vitamins C and K, fiber, and cancer-fighting compounds.','Cool-season vegetable that bolts in heat. Harvest central head when buds are tight. Side shoots continue producing smaller florets.'),
('Brussels Sprouts','Vegetable','Loamy','Brassica oleracea gemmifera',55,12,18,'Full Sun','Hard','Winter','High in fiber, vitamins K and C; improve with frost exposure.','Slow-growing cool-season crop taking 80-100 days. Flavor sweetens after frost. Remove lower leaves as sprouts develop upward.'),
('Buxus','Shrub','Loamy','Buxus sempervirens',40,12,18,'Partial Shade','Easy','Spring','Classic hedge and topiary plant; evergreen year-round structure.','Dense evergreen shrub perfect for formal hedges and topiaries. Responds well to frequent trimming. Slow-growing and very long-lived.'),
('Cabbage','Vegetable','Loamy','Brassica oleracea capitata',55,12,18,'Full Sun','Medium','Winter','Rich in vitamin C and fiber; versatile in cooking and fermenting.','Cool-season crop forming tight heads. Start indoors or direct sow in early spring. Consistent watering prevents head splitting.'),
('Cactus','Succulent','Sandy','Cactaceae',20,12,18,'Full Sun','Easy','Year-round','Extremely low maintenance; unique sculptural shapes for decor.','Desert plant family with thousands of species. Water sparingly, especially in winter. Needs excellent drainage. Many produce stunning flowers.'),
('Caladium','Indoor','Peaty','Caladium bicolor',45,12,18,'Partial Shade','Medium','Summer','Stunning heart-shaped colorful foliage for shaded gardens.','Tropical tuber grown for dramatic patterned leaves in pink, red, white, and green. Goes dormant in winter. Store tubers dry until spring.'),
('Calendula','Flowering','Loamy','Calendula officinalis',40,12,18,'Full Sun','Easy','Winter','Edible petals; natural skin healing; repels garden pests.','Cheerful orange and yellow flowers also called pot marigold. Edible petals add color to salads. Self-seeds readily for next season.'),
('Calathea','Indoor','Peaty','Calathea orbifolia',50,12,18,'Low Light','Medium','Year-round','Beautiful patterned foliage; leaves fold up at night like prayer.','Tropical plant with strikingly patterned leaves. Sensitive to tap water chemicals; use filtered water. Loves humidity above 50%.'),
('Camellia','Flowering','Acidic','Camellia japonica',50,12,18,'Partial Shade','Medium','Winter','Elegant winter flowers; source of tea; beautiful hedge plant.','Evergreen shrub blooming in late winter when little else flowers. Needs acidic soil and protection from morning sun. Long-lived specimen plant.'),
('Capsicum','Vegetable','Loamy','Capsicum annuum',50,12,18,'Full Sun','Medium','Summer','Rich in vitamin C; contains capsaicin with pain-relief properties.','Warm-season crop producing hot or sweet peppers. Start indoors in spring. Fruits change color as they ripen for increased sweetness.'),
('Carrot Plant','Vegetable','Sandy','Daucus carota',45,12,18,'Full Sun','Easy','Winter','Excellent source of beta-carotene for eye health and immunity.','Root vegetable that prefers loose, sandy soil for straight roots. Direct sow seeds as carrots dislike transplanting. Thin seedlings early.'),
('Cauliflower','Vegetable','Loamy','Brassica oleracea botrytis',55,12,18,'Full Sun','Hard','Winter','Low calorie, rich in vitamins C and K; great rice substitute.','Demanding cool-season crop that needs consistent conditions. Blanch heads by tying outer leaves over them. Harvest before curds separate.'),
('Celery','Vegetable','Loamy','Apium graveolens',50,12,18,'Full Sun','Hard','Winter','Low calorie snack; rich in vitamins and antioxidants; natural diuretic.','Long-season crop needing 130-140 days. Requires consistent moisture and rich soil. Blanch stalks by hilling soil around them.'),
('Cherry Plant','Fruit','Loamy','Prunus avium',70,12,18,'Full Sun','Hard','Winter','Delicious fruit rich in antioxidants; beautiful spring blossoms.','Deciduous fruit tree with stunning spring blossoms. Sweet cherries need a pollinizer variety. Takes 4-7 years to produce fruit.'),
('Chili Plant','Vegetable','Loamy','Capsicum frutescens',50,12,18,'Full Sun','Easy','Summer','Boosts metabolism; rich in vitamin C; natural pest deterrent.','Easy warm-season crop producing hot peppers. More stress and less water increases heat level. Prolific producers in containers.'),
('Chrysanthemum','Flowering','Loamy','Chrysanthemum indicum',45,12,18,'Full Sun','Medium','Monsoon','Natural insect repellent; used in teas; spectacular fall color.','Fall-blooming perennial with hundreds of varieties. Pinch stems in summer for bushier growth and more flowers. Short-day plant.'),
('Cilantro','Herb','Loamy','Coriandrum sativum',30,12,18,'Partial Shade','Easy','Winter','Essential herb in Asian and Mexican cuisine; aids digestion.','Cool-season herb that bolts quickly in heat. Succession plant every 3 weeks for continuous harvest. Seeds become coriander spice.'),
('Cinnamon Plant','Spice','Loamy','Cinnamomum verum',60,12,18,'Partial Shade','Hard','Monsoon','Bark yields cinnamon spice; anti-inflammatory and antimicrobial.','Tropical evergreen tree whose inner bark is the cinnamon spice. Can be container-grown indoors. Harvest bark from 2-year-old stems.'),
('Clove Plant','Spice','Loamy','Syzygium aromaticum',60,12,18,'Partial Shade','Hard','Monsoon','Antiseptic and pain-relieving; essential cooking spice worldwide.','Tropical evergreen tree producing aromatic flower buds. Takes 5-7 years to flower. Needs consistent warmth and humidity.'),
('Coleus','Indoor','Loamy','Plectranthus scutellarioides',40,12,18,'Partial Shade','Easy','Spring','Stunning multicolored foliage; easy to propagate from cuttings.','Vibrant foliage plant in endless color combinations. Pinch flowers to maintain leaf color. Root cuttings easily in water.'),
('Coriander','Herb','Loamy','Coriandrum sativum',30,12,18,'Partial Shade','Easy','Winter','Leaves and seeds both used as spice; rich in antioxidants.','Same plant as cilantro — leaves are cilantro, seeds are coriander. Best grown in cool weather. Let some plants go to seed for coriander harvest.'),
('Cosmos','Flowering','Loamy','Cosmos bipinnatus',40,12,18,'Full Sun','Easy','Summer','Attracts butterflies and bees; excellent cut flowers.','Cheerful daisy-like flowers on tall airy stems. Thrives in poor soil with minimal care. Self-seeds generously for next season.'),
('Croton','Indoor','Loamy','Codiaeum variegatum',50,12,18,'Indirect Light','Medium','Year-round','Vivid multicolored foliage adds tropical flair to any room.','Tropical plant with boldly colored leaves in red, orange, yellow, and green. Needs bright light to maintain color. Dislikes cold drafts.'),
('Cucumber Plant','Vegetable','Loamy','Cucumis sativus',60,12,18,'Full Sun','Easy','Summer','Hydrating vegetable with 95% water; refreshing in salads and drinks.','Fast-growing warm-season vine. Provide trellis for straighter fruits. Pick regularly to encourage continued production. Loves heat and moisture.'),
('Curry Leaf','Herb','Loamy','Murraya koenigii',55,12,18,'Full Sun','Medium','Monsoon','Essential in South Indian cooking; aids digestion; rich in iron.','Aromatic tree whose fresh leaves are irreplaceable in Indian cuisine. Grows well in containers. Use leaves fresh for best flavor.'),
('Cyclamen','Flowering','Peaty','Cyclamen persicum',45,12,18,'Indirect Light','Medium','Winter','Beautiful winter-blooming houseplant with swept-back petals.','Cool-loving plant that blooms in winter and goes dormant in summer. Water from below to avoid crown rot. Perfect for cool windowsills.'),
('Daffodil','Flowering','Loamy','Narcissus',40,12,18,'Full Sun','Easy','Winter','Cheerful spring flowers; deer and rodent resistant; naturalizes well.','Spring-blooming bulb that multiplies year after year. Plant bulbs in fall. Toxic to animals which makes them pest-proof.'),
('Dahlia','Flowering','Loamy','Dahlia pinnata',50,12,18,'Full Sun','Medium','Spring','Spectacular blooms in every color and size; excellent cut flowers.','Tuberous perennial with dinner-plate sized blooms. Dig up tubers after frost in cold climates. Pinch growing tips for bushier plants.'),
('Date Palm','Tree','Sandy','Phoenix dactylifera',80,12,18,'Full Sun','Hard','Spring','Sweet nutritious fruit; rich in fiber, potassium, and minerals.','Ancient desert tree producing sweet dates. Extremely heat and drought tolerant. Female trees need male pollinators. Takes 4-8 years to fruit.'),
('Dracaena','Indoor','Loamy','Dracaena fragrans',45,12,18,'Low Light','Easy','Year-round','Excellent air purifier removing formaldehyde and benzene.','Hardy indoor plant tolerating low light and irregular watering. Corn plant variety has yellow-striped leaves. Trim brown leaf tips.'),
('Eggplant','Vegetable','Loamy','Solanum melongena',60,12,18,'Full Sun','Medium','Summer','Rich in fiber and antioxidants; versatile in cooking worldwide.','Warm-season crop related to tomatoes. Needs long hot summers. Harvest while skin is still glossy. Available in purple, white, and striped varieties.'),
('Eucalyptus','Tree','Sandy','Eucalyptus globulus',75,12,18,'Full Sun','Medium','Spring','Aromatic leaves clear congestion; natural insect repellent; fast shade.','Fast-growing aromatic tree. Oil extracted from leaves used medicinally. Can be grown as a container plant and pruned to maintain size.'),
('Fern','Indoor','Peaty','Polypodiopsida',55,12,18,'Low Light','Medium','Year-round','Natural air humidifier; adds lush green texture to shaded areas.','Ancient plant group thriving in moist, shaded conditions. Boston ferns are popular houseplants. Mist regularly and avoid direct sunlight.'),
('Ficus','Indoor','Loamy','Ficus benjamina',50,12,18,'Indirect Light','Medium','Year-round','Excellent air purifier; elegant indoor tree for large spaces.','Popular indoor tree that drops leaves when stressed by changes. Once settled in a spot, avoid moving it. Can grow quite tall indoors.'),
('Fiddle Leaf Fig','Indoor','Loamy','Ficus lyrata',55,12,18,'Indirect Light','Hard','Spring','Statement houseplant; adds dramatic architectural element to rooms.','Trendy houseplant with large violin-shaped leaves. Sensitive to overwatering, cold drafts, and moving. Wipe leaves monthly for best health.'),
('Foxglove','Flowering','Loamy','Digitalis purpurea',45,12,18,'Partial Shade','Medium','Spring','Source of heart medicine digitalis; attracts hummingbirds; cottage garden staple.','Tall spikes of tubular flowers beloved by bees. Biennial that blooms second year. All parts are toxic if ingested. Self-seeds freely.'),
('Garlic Plant','Vegetable','Loamy','Allium sativum',45,12,18,'Full Sun','Easy','Winter','Powerful antimicrobial; boosts immunity; essential in global cuisine.','Plant individual cloves in fall for summer harvest. Hardneck varieties produce edible scapes. Cure bulbs in dry shade for 2 weeks after harvest.'),
('Geranium','Flowering','Loamy','Pelargonium',45,12,18,'Full Sun','Easy','Spring','Vibrant long-lasting blooms; mosquito-repelling scented varieties.','Classic container and bedding plant blooming all summer. Deadhead for continuous flowers. Overwinter indoors in cold climates. Easy from cuttings.'),
('Ginger Plant','Spice','Loamy','Zingiber officinale',55,12,18,'Partial Shade','Medium','Monsoon','Anti-nausea, anti-inflammatory; essential spice in Asian cooking.','Tropical rhizome harvested 8-10 months after planting. Prefers warm, humid, partly shaded conditions. Can be grown in containers indoors.'),
('Gladiolus','Flowering','Loamy','Gladiolus',50,12,18,'Full Sun','Easy','Spring','Tall dramatic spikes of color; excellent long-lasting cut flowers.','Plant corms in spring for summer blooms. Tall flower spikes need staking. Plant every 2 weeks for continuous blooms. Dig up corms in fall.'),
('Grapes Plant','Fruit','Loamy','Vitis vinifera',70,12,18,'Full Sun','Medium','Winter','Fresh eating, wine, raisins; rich in resveratrol antioxidant.','Deciduous vine needing strong support structure. Annual pruning essential for fruit production. Takes 2-3 years for first harvest.'),
('Guava Plant','Fruit','Loamy','Psidium guajava',75,12,18,'Full Sun','Easy','Monsoon','Extremely rich in vitamin C; aids digestion; delicious fruit.','Tropical fruit tree that fruits within 2-3 years. Can be container-grown. Tolerates brief cold spells. Fragrant flowers attract pollinators.'),
('Hibiscus','Flowering','Loamy','Hibiscus rosa-sinensis',55,12,18,'Full Sun','Easy','Summer','Large showy flowers; tea reduces blood pressure; attracts hummingbirds.','Tropical shrub with large colorful flowers lasting one day each. Prune in spring for bushier growth. Hibiscus tea is made from a different species.'),
('Hydrangea','Flowering','Acidic','Hydrangea macrophylla',60,12,18,'Partial Shade','Medium','Spring','Large flower clusters; color changes with soil pH; excellent dried flowers.','Shade-loving shrub with huge flower clusters. Blue flowers in acidic soil, pink in alkaline. Do not prune old-wood varieties in fall.'),
('Indian Gooseberry','Fruit','Loamy','Phyllanthus emblica',70,12,18,'Full Sun','Medium','Monsoon','Richest natural source of vitamin C; boosts immunity; Ayurvedic superfruit.','Deciduous tree producing tart green fruits called amla. Used extensively in Ayurvedic medicine. Fruits can be pickled, juiced, or dried.'),
('Ivy','Indoor','Loamy','Hedera helix',35,12,18,'Indirect Light','Easy','Year-round','Excellent air purifier; beautiful trailing plant for shelves.','Classic trailing plant perfect for hanging baskets. Can climb walls with aerial roots. Variegated varieties need more light. Trim to control growth.'),
('Jade Plant','Succulent','Sandy','Crassula ovata',30,12,18,'Indirect Light','Easy','Year-round','Symbol of good luck and prosperity; can live for decades.','Thick-stemmed succulent resembling a mini tree. Can live 50+ years with minimal care. Water only when soil is completely dry.'),
('Jasmine','Flowering','Loamy','Jasminum',45,12,18,'Full Sun','Medium','Spring','Intensely fragrant flowers; used in perfumes, teas, and garlands.','Climbing or shrubby plant with intensely sweet-scented white flowers. Night-blooming jasmine is especially fragrant. Prune after flowering.'),
('Kale','Vegetable','Loamy','Brassica oleracea acephala',50,12,18,'Full Sun','Easy','Winter','Superfood packed with vitamins A, C, K; more calcium than milk.','Hardy cool-season green that improves in flavor after frost. Harvest outer leaves first. Can grow through winter in many climates.'),
('Kiwi Plant','Fruit','Loamy','Actinidia deliciosa',75,12,18,'Full Sun','Hard','Spring','High in vitamin C and fiber; beautiful ornamental vine.','Vigorous deciduous vine needing strong support. Male and female plants needed for fruit. Takes 3-5 years to produce. Needs winter chill.'),
('Lavender','Herb','Sandy','Lavandula',40,12,18,'Full Sun','Easy','Spring','Calming fragrance reduces stress; natural moth repellent; culinary herb.','Fragrant Mediterranean herb loving sun and dry soil. Excellent in sachets, oils, and baking. Prune after flowering to prevent woodiness.'),
('Lemon Plant','Fruit','Loamy','Citrus limon',70,12,18,'Full Sun','Medium','Spring','Rich in vitamin C; versatile in cooking and cleaning; antibacterial.','Evergreen citrus tree that fruits year-round in warm climates. Dwarf varieties perfect for containers. Needs frost protection.'),
('Lettuce','Vegetable','Loamy','Lactuca sativa',45,12,18,'Partial Shade','Easy','Winter','Low calorie; hydrating; good source of vitamins A and K.','Fast-growing cool-season green ready in 30-60 days. Bolts in heat; grow in partial shade in summer. Harvest outer leaves for continuous picking.'),
('Lotus','Aquatic','Clay','Nelumbo nucifera',100,12,18,'Full Sun','Medium','Summer','Sacred flower; all parts edible; seeds viable for centuries.','Aquatic plant growing in still water. Roots anchored in mud with leaves and flowers above water. National flower of India. Seeds can remain viable for centuries.'),
('Mango Sapling','Fruit','Loamy','Mangifera indica',85,12,18,'Full Sun','Medium','Monsoon','King of fruits; rich in vitamins A and C; delicious flavor.','Tropical evergreen tree that can grow very large. Grafted trees fruit in 3-4 years. Needs hot dry period to trigger flowering.'),
('Marigold','Flowering','Loamy','Tagetes',40,12,18,'Full Sun','Easy','Monsoon','Natural pest repellent for vegetable gardens; used in religious ceremonies.','Cheerful orange and yellow flowers blooming all season. Repels nematodes and many insects when planted near vegetables. Extremely easy to grow from seed.'),
('Mint','Herb','Loamy','Mentha',35,12,18,'Partial Shade','Easy','Year-round','Aids digestion; freshens breath; cooling in drinks and chutneys.','Vigorous herb that spreads aggressively — grow in containers! Many varieties: peppermint, spearmint, chocolate mint. Harvest often for bushiest growth.'),
('Money Plant','Indoor','Loamy','Epipremnum aureum',40,12,18,'Low Light','Easy','Year-round','Excellent air purifier; believed to bring prosperity; nearly unkillable.','One of the easiest houseplants, tolerating low light and neglect. Grows in water or soil. Trailing vine perfect for shelves and hanging baskets.'),
('Mulberry','Fruit','Loamy','Morus alba',70,12,18,'Full Sun','Easy','Spring','Sweet berries rich in antioxidants; leaves feed silkworms; fast growing.','Fast-growing deciduous tree producing sweet berries. Stains surfaces, so plant away from walkways. Very productive with minimal care.'),
('Neem','Tree','Loamy','Azadirachta indica',70,12,18,'Full Sun','Easy','Monsoon','Natural pesticide; antibacterial; used in toothpaste and skincare.','Sacred Indian tree with powerful medicinal properties. Neem oil is a natural pesticide. Extremely hardy and drought-tolerant once established.'),
('Okra Plant','Vegetable','Loamy','Abelmoschus esculentus',60,12,18,'Full Sun','Easy','Summer','Rich in fiber, vitamin C, and folate; beautiful hibiscus-like flowers.','Heat-loving vegetable related to hibiscus. Produces edible pods in 50-60 days. Harvest young pods daily for tenderness. Attractive yellow flowers.'),
('Olive Plant','Fruit','Sandy','Olea europaea',65,12,18,'Full Sun','Medium','Spring','Heart-healthy oil; symbol of peace; extremely long-lived tree.','Evergreen Mediterranean tree living for centuries. Drought-tolerant once established. Needs dry climate for fruit production. Beautiful silver foliage.'),
('Onion Plant','Vegetable','Loamy','Allium cepa',45,12,18,'Full Sun','Easy','Winter','Essential kitchen staple; prebiotic fiber; natural antibiotic properties.','Bulb vegetable that grows according to day length. Long-day varieties for north, short-day for south. Harvest when tops fall over naturally.'),
('Orange Plant','Fruit','Loamy','Citrus sinensis',70,12,18,'Full Sun','Medium','Spring','Excellent vitamin C source; fragrant blossoms; juicing and eating.','Evergreen citrus tree with fragrant white flowers. Dwarf varieties suitable for containers. Needs protection from frost. Fruits need months to ripen.'),
('Orchid','Flowering','Bark Mix','Orchidaceae',45,12,18,'Indirect Light','Medium','Year-round','Elegant long-lasting flowers; wide variety of colors and patterns.','Diverse plant family with over 25,000 species. Phalaenopsis (moth orchid) is easiest for beginners. Water weekly with ice cubes. Blooms last months.'),
('Oregano','Herb','Loamy','Origanum vulgare',30,12,18,'Full Sun','Easy','Spring','Essential pizza and pasta herb; rich in antioxidants; antimicrobial.','Mediterranean herb with strong flavor improving when dried. Cut stems just above a leaf pair. Hardy perennial in warm climates.'),
('Papaya Plant','Fruit','Loamy','Carica papaya',80,12,18,'Full Sun','Medium','Monsoon','Rich in papain enzyme aiding digestion; high in vitamins A and C.','Fast-growing tropical tree fruiting within a year. Not frost-tolerant. Some are self-pollinating but most need separate male and female plants.'),
('Parsley','Herb','Loamy','Petroselinum crispum',35,12,18,'Partial Shade','Easy','Winter','Rich in vitamins K and C; freshens breath; versatile garnish.','Biennial herb usually grown as annual. Flat-leaf has stronger flavor; curly is decorative. Slow to germinate — soak seeds before planting.'),
('Peace Lily','Indoor','Peaty','Spathiphyllum',50,12,18,'Low Light','Easy','Year-round','Top air purifier; elegant white blooms; thrives in low light.','One of few plants that bloom in low light. White spathe flowers appear several times a year. Droops dramatically when thirsty but recovers quickly.'),
('Peach Plant','Fruit','Loamy','Prunus persica',75,12,18,'Full Sun','Hard','Winter','Juicy sweet fruit; rich in vitamins A and C; beautiful spring blossoms.','Deciduous fruit tree needing cold winters. Self-pollinating varieties available. Thin fruits for larger peaches. Takes 2-4 years to bear fruit.'),
('Peanut Plant','Vegetable','Sandy','Arachis hypogaea',50,12,18,'Full Sun','Medium','Summer','Protein-rich legume; fixes nitrogen in soil; versatile food source.','Unique plant whose flowers push into soil to form underground peanuts. Needs 120-150 warm days. Harvest when leaves yellow in fall.'),
('Peperomia','Indoor','Loamy','Peperomia obtusifolia',35,12,18,'Indirect Light','Easy','Year-round','Compact pet-safe houseplant; attractive waxy foliage; air purifying.','Compact tropical plant with thick waxy leaves. Over 1000 species with diverse leaf shapes. Semi-succulent so avoid overwatering.'),
('Petunia','Flowering','Loamy','Petunia hybrida',40,12,18,'Full Sun','Easy','Spring','Prolific colorful blooms all season; fragrant evening varieties.','Popular annual in every color imaginable. Cascading types perfect for hanging baskets. Pinch leggy stems for fuller growth. Feed weekly for best blooms.'),
('Philodendron','Indoor','Loamy','Philodendron',40,12,18,'Indirect Light','Easy','Year-round','Excellent air purifier; easy-care climbing plant; many varieties.','Large genus of tropical plants ranging from vining to tree-like. Heart-leaf philodendron is easiest. Fast-growing and forgiving of neglect.'),
('Pineapple Plant','Fruit','Sandy','Ananas comosus',65,12,18,'Full Sun','Medium','Summer','Bromelain enzyme aids digestion; rich in vitamin C and manganese.','Tropical fruit grown from the crown of a store-bought pineapple. Takes 18-24 months to fruit. Each plant produces just one fruit then sends up offsets.'),
('Pothos','Indoor','Loamy','Epipremnum pinnatum',40,12,18,'Low Light','Easy','Year-round','Top NASA air purifier; nearly impossible to kill; beautiful trailer.','Similar to money plant. Extremely forgiving houseplant thriving in low light. Grows in water or soil. Many varieties with different leaf patterns.'),
('Radish Plant','Vegetable','Sandy','Raphanus sativus',40,12,18,'Full Sun','Easy','Winter','Fastest growing vegetable; ready in 25 days; aids digestion.','One of the fastest vegetables — some varieties ready in 25 days! Direct sow seeds. Cool-season crop. Great for beginner gardeners and kids.'),
('Rose','Flowering','Loamy','Rosa',50,12,18,'Full Sun','Medium','Winter','Queen of flowers; fragrance used in perfumes; petals are edible.','Beloved flowering shrub with thousands of varieties. Prune in late winter. Feed monthly during growing season. Watch for black spot and aphids.'),
('Rosemary','Herb','Sandy','Salvia rosmarinus',35,12,18,'Full Sun','Easy','Spring','Improves memory; aromatic culinary herb; natural air freshener.','Woody Mediterranean herb with needle-like leaves. Drought-tolerant once established. Excellent in containers. Flavor pairs with roasted meats and potatoes.'),
('Rubber Plant','Indoor','Loamy','Ficus elastica',50,12,18,'Indirect Light','Easy','Year-round','Excellent air purifier; large glossy leaves make bold statement.','Hardy indoor plant with thick glossy leaves. Wipe leaves with damp cloth monthly. Prune to control size and shape. Milky sap can irritate skin.'),
('Sage','Herb','Loamy','Salvia officinalis',35,12,18,'Full Sun','Easy','Spring','Anti-inflammatory; improves memory; essential in stuffing and sausages.','Woody perennial herb with velvety grey-green leaves. Flavor is strong so use sparingly. Beautiful purple flowers attract bees. Drought-tolerant.'),
('Snake Plant','Indoor','Sandy','Dracaena trifasciata',40,12,18,'Low Light','Easy','Year-round','Produces oxygen at night; top air purifier; nearly indestructible.','One of the toughest houseplants alive. Tolerates low light and weeks without water. Converts CO2 to oxygen at night — perfect for bedrooms.'),
('Spinach','Vegetable','Loamy','Spinacia oleracea',45,12,18,'Partial Shade','Easy','Winter','Iron-rich superfood; packed with vitamins A, C, and K.','Fast-growing cool-season green ready in 35-45 days. Bolts in heat so grow in spring/fall. Harvest outer leaves for continuous picking.'),
('Spider Plant','Indoor','Loamy','Chlorophytum comosum',45,12,18,'Indirect Light','Easy','Year-round','Great air purifier; safe for pets; produces baby plantlets.','Classic houseplant producing cascading baby plantlets on runners. Extremely adaptable and forgiving. Brown leaf tips mean fluoride in water.'),
('Strawberry Plant','Fruit','Loamy','Fragaria × ananassa',55,12,18,'Full Sun','Easy','Winter','Sweet antioxidant-rich fruit; easy to grow in containers.','Perennial fruit plant producing sweet berries. June-bearing types give one large harvest; ever-bearing produce all season. Runners create new plants.'),
('Succulent','Succulent','Sandy','Succulentus',25,12,18,'Full Sun','Easy','Year-round','Low maintenance decor; stores water in leaves; hundreds of varieties.','Broad group of water-storing plants in many shapes and colors. Need bright light and infrequent watering. Perfect for sunny windowsills and gifts.'),
('Sunflower','Flowering','Loamy','Helianthus annuus',60,12,18,'Full Sun','Easy','Summer','Edible seeds rich in vitamin E; attracts pollinators; cheerful blooms.','Iconic tall flower that follows the sun. Easy to grow from seed. Dwarf varieties for containers. Seeds are nutritious snacks for humans and birds.'),
('Thyme','Herb','Sandy','Thymus vulgaris',30,12,18,'Full Sun','Easy','Spring','Antiseptic properties; essential herb in French cuisine; attracts bees.','Low-growing aromatic herb ideal for borders and rock gardens. Dozens of varieties with different scents. Drought-tolerant and deer-resistant.'),
('Tomato Plant','Vegetable','Loamy','Solanum lycopersicum',60,12,18,'Full Sun','Medium','Summer','Rich in lycopene antioxidant; versatile in cooking; easy to grow.','Most popular home garden vegetable. Determinate types are compact; indeterminate types keep growing. Stake or cage for support. Feed regularly.'),
('Tulsi','Herb','Loamy','Ocimum tenuiflorum',35,12,18,'Full Sun','Easy','Monsoon','Sacred herb in Hinduism; boosts immunity; natural stress reliever.','Holy basil revered in Indian culture. Plant near entrances for its purifying properties. Different varieties: Rama, Krishna, Vana. Harvest leaves regularly.'),
('Vanilla Plant','Spice','Loamy','Vanilla planifolia',60,12,18,'Partial Shade','Hard','Monsoon','Source of natural vanilla; second most expensive spice after saffron.','Tropical orchid vine producing vanilla bean pods. Must be hand-pollinated outside Mexico. Takes 3-5 years to flower. Needs warm humid conditions.'),
('Watermelon Plant','Fruit','Sandy','Citrullus lanatus',70,12,18,'Full Sun','Medium','Summer','92% water for hydration; rich in lycopene and citrulline.','Sprawling vine needing lots of space and heat. Ready in 80-100 days. Thump for hollow sound to check ripeness. Needs consistent deep watering.'),
('Wheatgrass','Herb','Loamy','Triticum aestivum',35,12,18,'Indirect Light','Easy','Year-round','Nutrient-dense superfood juice; grows in 7-10 days; detoxifying.','Young wheat plant harvested at 7-10 days old for juicing. Grow on trays indoors year-round. Rich in chlorophyll, vitamins, and minerals.'),
('ZZ Plant','Indoor','Loamy','Zamioculcas zamiifolia',35,12,18,'Low Light','Easy','Year-round','Thrives on neglect; glossy foliage; excellent for offices and dark rooms.','One of the most drought-tolerant houseplants. Thick rhizomes store water. Tolerates low light, dry air, and irregular watering. Slow growing but very hardy.');
