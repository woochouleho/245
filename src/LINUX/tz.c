#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tz.h"
#include "./form_src/multilang.h"

/* This table is from http://wiki.openwrt.org/doc/uci/system#time.zones,
 * we should update this table from the site regularly */
static struct tz {
	const int location;
	const char *string, *location_cli;
} tz_db[] = {
	{ LANG_AFRICA_ABIDJAN, "GMT0", "Africa/Abidjan"},	
	{ LANG_AFRICA_ACCRA, "GMT0", "Africa/Accra"},	
	{ LANG_AFRICA_ADDIS_ABABA, "EAT-3", "Africa/Addis Ababa"},	
	{ LANG_AFRICA_ALGIERS, "CET-1", "Africa/Algiers"},	
	{ LANG_AFRICA_ASMARA, "EAT-3", "Africa/Asmara"},	
	{ LANG_AFRICA_BAMAKO, "GMT0", "Africa/Bamako"},	
	{ LANG_AFRICA_BANGUI, "WAT-1", "Africa/Bangui"},	
	{ LANG_AFRICA_BANJUL, "GMT0", "Africa/Banjul"},	
	{ LANG_AFRICA_BISSAU, "GMT0", "Africa/Bissau"},	
	{ LANG_AFRICA_BLANTYRE, "CAT-2", "Africa/Blantyre"},	
	{ LANG_AFRICA_BRAZZAVILLE, "WAT-1", "Africa/Brazzaville"},	
	{ LANG_AFRICA_BUJUMBURA, "CAT-2", "Africa/Bujumbura"},	
	{ LANG_AFRICA_CASABLANCA, "WET0", "Africa/Casablanca"},	
	{ LANG_AFRICA_CEUTA, "CET-1CEST,M3.5.0,M10.5.0/3", "Africa/Ceuta"},	
	{ LANG_AFRICA_CONAKRY, "GMT0", "Africa/Conakry"},	
	{ LANG_AFRICA_DAKAR, "GMT0", "Africa/Dakar"},	
	{ LANG_AFRICA_DAR_ES_SALAAM, "EAT-3", "Africa/Dar es Salaam"},	
	{ LANG_AFRICA_DJIBOUTI, "EAT-3", "Africa/Djibouti"},	
	{ LANG_AFRICA_DOUALA, "WAT-1", "Africa/Douala"},	
	{ LANG_AFRICA_EL_AAIUN, "WET0", "Africa/El Aaiun"},	
	{ LANG_AFRICA_FREETOWN, "GMT0", "Africa/Freetown"},	
	{ LANG_AFRICA_GABORONE, "CAT-2", "Africa/Gaborone"},	
	{ LANG_AFRICA_HARARE, "CAT-2", "Africa/Harare"},	
	{ LANG_AFRICA_JOHANNESBURG, "SAST-2", "Africa/Johannesburg"},	
	{ LANG_AFRICA_KAMPALA, "EAT-3", "Africa/Kampala"},	
	{ LANG_AFRICA_KHARTOUM, "EAT-3", "Africa/Khartoum"},	
	{ LANG_AFRICA_KIGALI, "CAT-2", "Africa/Kigali"},	
	{ LANG_AFRICA_KINSHASA, "WAT-1", "Africa/Kinshasa"},	
	{ LANG_AFRICA_LAGOS, "WAT-1", "Africa/Lagos"},	
	{ LANG_AFRICA_LIBREVILLE, "WAT-1", "Africa/Libreville"},	
	{ LANG_AFRICA_LOME, "GMT0", "Africa/Lome"},	
	{ LANG_AFRICA_LUANDA, "WAT-1", "Africa/Luanda"},	
	{ LANG_AFRICA_LUBUMBASHI, "CAT-2", "Africa/Lubumbashi"},	
	{ LANG_AFRICA_LUSAKA, "CAT-2", "Africa/Lusaka"},	
	{ LANG_AFRICA_MALABO, "WAT-1", "Africa/Malabo"},	
	{ LANG_AFRICA_MAPUTO, "CAT-2", "Africa/Maputo"},	
	{ LANG_AFRICA_MASERU, "SAST-2", "Africa/Maseru"},	
	{ LANG_AFRICA_MBABANE, "SAST-2", "Africa/Mbabane"},	
	{ LANG_AFRICA_MOGADISHU, "EAT-3", "Africa/Mogadishu"},	
	{ LANG_AFRICA_MONROVIA, "GMT0", "Africa/Monrovia"},	
	{ LANG_AFRICA_NAIROBI, "EAT-3", "Africa/Nairobi"},	
	{ LANG_AFRICA_NDJAMENA, "WAT-1", "Africa/Ndjamena"},	
	{ LANG_AFRICA_NIAMEY, "WAT-1", "Africa/Niamey"},	
	{ LANG_AFRICA_NOUAKCHOTT, "GMT0", "Africa/Nouakchott"},	
	{ LANG_AFRICA_OUAGADOUGOU, "GMT0", "Africa/Ouagadougou"},	
	{ LANG_AFRICA_PORTO_NOVO, "WAT-1", "Africa/Porto-Novo"},	
	{ LANG_AFRICA_SAO_TOME, "GMT0", "Africa/Sao Tome"},	
	{ LANG_AFRICA_TRIPOLI, "EET-2", "Africa/Tripoli"},	
	{ LANG_AFRICA_TUNIS, "CET-1", "Africa/Tunis"},	
	{ LANG_AFRICA_WINDHOEK, "WAT-1WAST,M9.1.0,M4.1.0", "Africa/Windhoek"},	
	{ LANG_AMERICA_ADAK, "HAST10HADT,M3.2.0,M11.1.0", "America/Adak"},	
	{ LANG_AMERICA_ANCHORAGE, "	AKST9AKDT,M3.2.0,M11.1.0", "America/Anchorage"},	
	{ LANG_AMERICA_ANGUILLA, "AST4", "America/Anguilla"},	
	{ LANG_AMERICA_ANTIGUA, "AST4", "America/Antigua"},	
	{ LANG_AMERICA_ARAGUAINA, "BRT3", "America/Araguaina"},	
	{ LANG_AMERICA_ARGENTINA_BUENOS_AIRES, "ART3", "America/Argentina/Buenos Aires"},	
	{ LANG_AMERICA_ARGENTINA_CATAMARCA, "ART3", "America/Argentina/Catamarca"},	
	{ LANG_AMERICA_ARGENTINA_CORDOBA, "ART3", "America/Argentina/Cordoba"},	
	{ LANG_AMERICA_ARGENTINA_JUJUY, "ART3", "America/Argentina/Jujuy"},	
	{ LANG_AMERICA_ARGENTINA_LA_RIOJA, "ART3", "America/Argentina/La Rioja"},	
	{ LANG_AMERICA_ARGENTINA_MENDOZA, "ART3", "America/Argentina/Mendoza"},	
	{ LANG_AMERICA_ARGENTINA_RIO_GALLEGOS, "ART3", "America/Argentina/Rio Gallegos"},	
	{ LANG_AMERICA_ARGENTINA_SALTA, "ART3", "America/Argentina/Salta"},	
	{ LANG_AMERICA_ARGENTINA_SAN_JUAN, "ART3", "America/Argentina/San Juan"},	
	{ LANG_AMERICA_ARGENTINA_TUCUMAN, "ART3", "America/Argentina/Tucuman"},	
	{ LANG_AMERICA_ARGENTINA_USHUAIA, "ART3", "America/Argentina/Ushuaia"},	
	{ LANG_AMERICA_ARUBA, "AST4", "America/Aruba"},	
	{ LANG_AMERICA_ASUNCION, "PYT4PYST,M10.1.0/0,M4.2.0/0", "America/Asuncion"},	
	{ LANG_AMERICA_ATIKOKAN, "EST5", "America/Atikokan"},	
	{ LANG_AMERICA_BAHIA, "BRT3", "America/Bahia"},	
	{ LANG_AMERICA_BARBADOS, "AST4", "America/Barbados"},	
	{ LANG_AMERICA_BELEM, "BRT3", "America/Belem"},	
	{ LANG_AMERICA_BELIZE, "CST6", "America/Belize"},	
	{ LANG_AMERICA_BLANC_SABLON, "AST4", "America/Blanc-Sablon"},	
	{ LANG_AMERICA_BOA_VISTA, "AMT4", "America/Boa Vista"},	
	{ LANG_AMERICA_BOGOTA, "COT5", "America/Bogota"},	
	{ LANG_AMERICA_BOISE, "	MST7MDT,M3.2.0,M11.1.0", "America/Boise"},	
	{ LANG_AMERICA_CAMBRIDGE_BAY, "MST7MDT,M3.2.0,M11.1.0", "America/Cambridge Bay"},	
	{ LANG_AMERICA_CAMPO_GRANDE, "AMT4AMST,M10.3.0/0,M2.3.0/0", "America/Campo Grande"},	
	{ LANG_AMERICA_CANCUN, "CST6CDT,M4.1.0,M10.5.0", "America/Cancun"},	
	{ LANG_AMERICA_CARACAS, "VET4:30", "America/Caracas"},	
	{ LANG_AMERICA_CAYENNE, "GFT3", "America/Cayenne"},	
	{ LANG_AMERICA_CAYMAN, "EST5", "America/Cayman"},	
	{ LANG_AMERICA_CHICAGO, "CST6CDT,M3.2.0,M11.1.0", "America/Chicago"},	
	{ LANG_AMERICA_CHIHUAHUA, "MST7MDT,M4.1.0,M10.5.0", "America/Chihuahua"},	
	{ LANG_AMERICA_COSTA_RICA, "CST6", "America/Costa Rica"},	
	{ LANG_AMERICA_CUIABA, "AMT4AMST,M10.3.0/0,M2.3.0/0", "America/Cuiaba"},	
	{ LANG_AMERICA_CURACAO, "AST4", "America/Curacao"},	
	{ LANG_AMERICA_DANMARKSHAVN, "GMT0", "America/Danmarkshavn"},	
	{ LANG_AMERICA_DAWSON, "PST8PDT,M3.2.0,M11.1.0", "America/Dawson"},	
	{ LANG_AMERICA_DAWSON_CREEK, "MST7", "America/Dawson Creek"},	
	{ LANG_AMERICA_DENVER, "MST7MDT,M3.2.0,M11.1.0", "America/Denver"},	
	{ LANG_AMERICA_DETROIT, "EST5EDT,M3.2.0,M11.1.0", "America/Detroit"},	
	{ LANG_AMERICA_DOMINICA, "AST4", "America/Dominica"},	
	{ LANG_AMERICA_EDMONTON, "MST7MDT,M3.2.0,M11.1.0", "America/Edmonton"},	
	{ LANG_AMERICA_EIRUNEPE, "AMT4", "America/Eirunepe"},	
	{ LANG_AMERICA_EL_SALVADOR, "CST6", "America/El Salvador"},	
	{ LANG_AMERICA_FORTALEZA, "BRT3", "America/Fortaleza"},	
	{ LANG_AMERICA_GLACE_BAY, "AST4ADT,M3.2.0,M11.1.0", "America/Glace Bay"},	
	{ LANG_AMERICA_GOOSE_BAY, "AST4ADT,M3.2.0,M11.1.0", "America/Goose Bay"},	
	{ LANG_AMERICA_GRAND_TURK, "EST5EDT,M3.2.0,M11.1.0", "America/Grand Turk"},	
	{ LANG_AMERICA_GRENADA, "AST4", "America/Grenada"},	
	{ LANG_AMERICA_GUADELOUPE, "AST4", "America/Guadeloupe"},	
	{ LANG_AMERICA_GUATEMALA, "CST6", "America/Guatemala"},	
	{ LANG_AMERICA_GUAYAQUIL, "ECT5", "America/Guayaquil"},	
	{ LANG_AMERICA_GUYANA, "GYT4", "America/Guyana"},	
	{ LANG_AMERICA_HALIFAX, "AST4ADT,M3.2.0,M11.1.0", "America/Halifax"},	
	{ LANG_AMERICA_HAVANA, "CST5CDT,M3.2.0/0,M10.5.0/1", "America/Havana"},	
	{ LANG_AMERICA_HERMOSILLO, "MST7", "America/Hermosillo"},	
	{ LANG_AMERICA_INDIANA_INDIANAPOLIS, "EST5EDT,M3.2.0,M11.1.0", "America/Indiana/Indianapolis"},	
	{ LANG_AMERICA_INDIANA_KNOX, "CST6CDT,M3.2.0,M11.1.0", "America/Indiana/Knox"},	
	{ LANG_AMERICA_INDIANA_MARENGO, "EST5EDT,M3.2.0,M11.1.0", "America/Indiana/Marengo"},	
	{ LANG_AMERICA_INDIANA_PETERSBURG, "EST5EDT,M3.2.0,M11.1.0", "America/Indiana/Petersburg"},	
	{ LANG_AMERICA_INDIANA_TELL_CITY, "CST6CDT,M3.2.0,M11.1.0", "America/Indiana/Tell City"},	
	{ LANG_AMERICA_INDIANA_VEVAY, "EST5EDT,M3.2.0,M11.1.0", "America/Indiana/Vevay"},	
	{ LANG_AMERICA_INDIANA_VINCENNES, "EST5EDT,M3.2.0,M11.1.0", "America/Indiana/Vincennes"},	
	{ LANG_AMERICA_INDIANA_WINAMAC, "EST5EDT,M3.2.0,M11.1.0", "America/Indiana/Winamac"},	
	{ LANG_AMERICA_INUVIK, "MST7MDT,M3.2.0,M11.1.0", "America/Inuvik"},	
	{ LANG_AMERICA_IQALUIT, "EST5EDT,M3.2.0,M11.1.0", "America/Iqaluit"},	
	{ LANG_AMERICA_JAMAICA, "EST5", "America/Jamaica"},	
	{ LANG_AMERICA_JUNEAU, "AKST9AKDT,M3.2.0,M11.1.0", "America/Juneau"},	
	{ LANG_AMERICA_KENTUCKY_LOUISVILLE, "EST5EDT,M3.2.0,M11.1.0", "America/Kentucky/Louisville"},	
	{ LANG_AMERICA_KENTUCKY_MONTICELLO, "EST5EDT,M3.2.0,M11.1.0", "America/Kentucky/Monticello"},	
	{ LANG_AMERICA_LA_PAZ, "BOT4", "America/La Paz"},	
	{ LANG_AMERICA_LIMA, "PET5", "America/Lima"},	
	{ LANG_AMERICA_LOS_ANGELES, "PST8PDT,M3.2.0,M11.1.0", "America/Los Angeles"},	
	{ LANG_AMERICA_MACEIO, "BRT3", "America/Maceio"},	
	{ LANG_AMERICA_MANAGUA, "CST6", "America/Managua"},	
	{ LANG_AMERICA_MANAUS, "AMT4", "America/Manaus"},	
	{ LANG_AMERICA_MARIGOT, "AST4", "America/Marigot"},	
	{ LANG_AMERICA_MARTINIQUE, "AST4", "America/Martinique"},	
	{ LANG_AMERICA_MATAMOROS, "CST6CDT,M3.2.0,M11.1.0", "America/Matamoros"},	
	{ LANG_AMERICA_MAZATLAN, "MST7MDT,M4.1.0,M10.5.0", "America/Mazatlan"},	
	{ LANG_AMERICA_MENOMINEE, "CST6CDT,M3.2.0,M11.1.0", "America/Menominee"},	
	{ LANG_AMERICA_MERIDA, "CST6CDT,M4.1.0,M10.5.0", "America/Merida"},	
	{ LANG_AMERICA_MEXICO_CITY, "CST6CDT,M4.1.0,M10.5.0", "America/Mexico City"},	
	{ LANG_AMERICA_MIQUELON, "PMST3PMDT,M3.2.0,M11.1.0", "America/Miquelon"},	
	{ LANG_AMERICA_MONCTON, "AST4ADT,M3.2.0,M11.1.0", "America/Moncton"},	
	{ LANG_AMERICA_MONTERREY, "CST6CDT,M4.1.0,M10.5.0", "America/Monterrey"},	
	{ LANG_AMERICA_MONTEVIDEO, "UYT3UYST,M10.1.0,M3.2.0", "America/Montevideo"},	
	{ LANG_AMERICA_MONTREAL, "EST5EDT,M3.2.0,M11.1.0", "America/Montreal"},	
	{ LANG_AMERICA_MONTSERRAT, "AST4", "America/Montserrat"},	
	{ LANG_AMERICA_NASSAU, "EST5EDT,M3.2.0,M11.1.0", "America/Nassau"},	
	{ LANG_AMERICA_NEW_YORK, "EST5EDT,M3.2.0,M11.1.0", "America/New York"},	
	{ LANG_AMERICA_NIPIGON, "EST5EDT,M3.2.0,M11.1.0", "America/Nipigon"},	
	{ LANG_AMERICA_NOME, "AKST9AKDT,M3.2.0,M11.1.0", "America/Nome"},	
	{ LANG_AMERICA_NORONHA, "FNT2", "America/Noronha"},	
	{ LANG_AMERICA_NORTH_DAKOTA_CENTER, "CST6CDT,M3.2.0,M11.1.0", "America/North Dakota/Center"},	
	{ LANG_AMERICA_NORTH_DAKOTA_NEW_SALEM, "CST6CDT,M3.2.0,M11.1.0", "America/North Dakota/New Salem"},	
	{ LANG_AMERICA_OJINAGA, "MST7MDT,M3.2.0,M11.1.0", "America/Ojinaga"},	
	{ LANG_AMERICA_PANAMA, "EST5", "America/Panama"},	
	{ LANG_AMERICA_PANGNIRTUNG, "EST5EDT,M3.2.0,M11.1.0", "America/Pangnirtung"},	
	{ LANG_AMERICA_PARAMARIBO, "SRT3", "America/Paramaribo"},	
	{ LANG_AMERICA_PHOENIX, "MST7", "America/Phoenix"},	
	{ LANG_AMERICA_PORT_OF_SPAIN, "AST4", "America/Port of Spain"},	
	{ LANG_AMERICA_PORT_AU_PRINCE, "EST5", "America/Port-au-Prince"},	
	{ LANG_AMERICA_PORTO_VELHO, "AMT4", "America/Porto Velho"},	
	{ LANG_AMERICA_PUERTO_RICO, "AST4", "America/Puerto Rico"},	
	{ LANG_AMERICA_RAINY_RIVER, "CST6CDT,M3.2.0,M11.1.0", "America/Rainy River"},	
	{ LANG_AMERICA_RANKIN_INLET, "CST6CDT,M3.2.0,M11.1.0", "America/Rankin Inlet"},	
	{ LANG_AMERICA_RECIFE, "BRT3", "America/Recife"},	
	{ LANG_AMERICA_REGINA, "CST6", "America/Regina"},	
	{ LANG_AMERICA_RIO_BRANCO, "AMT4", "America/Rio Branco"},	
	{ LANG_AMERICA_SANTA_ISABEL, "PST8PDT,M4.1.0,M10.5.0", "America/Santa Isabel"},	
	{ LANG_AMERICA_SANTAREM, "BRT3", "America/Santarem"},	
	{ LANG_AMERICA_SANTO_DOMINGO, "AST4", "America/Santo Domingo"},	
	{ LANG_AMERICA_SAO_PAULO, "PST8PDT,M4.1.0,M10.5.0", "America/Sao Paulo"},	
	{ LANG_AMERICA_SCORESBYSUND, "EGT1EGST,M3.5.0/0,M10.5.0/1", "America/Scoresbysund"},	
	{ LANG_AMERICA_SHIPROCK, "MST7MDT,M3.2.0,M11.1.0", "America/Shiprock"},	
	{ LANG_AMERICA_ST_BARTHELEMY, "AST4", "America/St Barthelemy"},	
	{ LANG_AMERICA_ST_JOHNS, "NST3:30NDT,M3.2.0/0:01,M11.1.0/0:01", "America/St Johns"},	
	{ LANG_AMERICA_ST_KITTS, "AST4", "America/St Kitts"},	
	{ LANG_AMERICA_ST_LUCIA, "AST4", "America/St Lucia"},	
	{ LANG_AMERICA_ST_THOMAS, "AST4", "America/St Thomas"},	
	{ LANG_AMERICA_ST_VINCENT, "AST4", "America/St Vincent"},	
	{ LANG_AMERICA_SWIFT_CURRENT, "CST6", "America/Swift Current"},	
	{ LANG_AMERICA_TEGUCIGALPA, "CST6", "America/Tegucigalpa"},	
	{ LANG_AMERICA_THULE, "AST4ADT,M3.2.0,M11.1.0", "America/Thule"},	
	{ LANG_AMERICA_THUNDER_BAY, "EST5EDT,M3.2.0,M11.1.0", "America/Thunder Bay"},	
	{ LANG_AMERICA_TIJUANA, "PST8PDT,M3.2.0,M11.1.0", "America/Tijuana"},	
	{ LANG_AMERICA_TORONTO, "EST5EDT,M3.2.0,M11.1.0", "America/Toronto"},	
	{ LANG_AMERICA_TORTOLA, "AST4", "America/Tortola"},	
	{ LANG_AMERICA_VANCOUVER, "PST8PDT,M3.2.0,M11.1.0", "America/Vancouver"},	
	{ LANG_AMERICA_WHITEHORSE, "PST8PDT,M3.2.0,M11.1.0", "America/Whitehorse"},	
	{ LANG_AMERICA_WINNIPEG, "CST6CDT,M3.2.0,M11.1.0", "America/Winnipeg"},	
	{ LANG_AMERICA_YAKUTAT, "AKST9AKDT,M3.2.0,M11.1.0", "America/Yakutat"},	
	{ LANG_AMERICA_YELLOWKNIFE, "MST7MDT,M3.2.0,M11.1.0", "America/Yellowknife"},	
	{ LANG_ANTARCTICA_CASEY, "WST-8", "Antarctica/Casey"},	
	{ LANG_ANTARCTICA_DAVIS, "DAVT-7", "Antarctica/Davis"},	
	{ LANG_ANTARCTICA_DUMONTDURVILLE, "DDUT-10", "Antarctica/DumontDUrville"},	
	{ LANG_ANTARCTICA_MACQUARIE, "MIST-11", "Antarctica/Macquarie"},	
	{ LANG_ANTARCTICA_MAWSON, "MAWT-5", "Antarctica/Mawson"},	
	{ LANG_ANTARCTICA_MCMURDO, "NZST-12NZDT,M9.5.0,M4.1.0/3", "Antarctica/McMurdo"},	
	{ LANG_ANTARCTICA_ROTHERA, "ROTT3", "Antarctica/Rothera"},	
	{ LANG_ANTARCTICA_SOUTH_POLE, "NZST-12NZDT,M9.5.0,M4.1.0/3", "Antarctica/South Pole"},	
	{ LANG_ANTARCTICA_SYOWA, "SYOT-3", "Antarctica/Syowa"},	
	{ LANG_ANTARCTICA_VOSTOK, "VOST-6", "Antarctica/Vostok"},	
	{ LANG_ARCTIC_LONGYEARBYEN, "CET-1CEST,M3.5.0,M10.5.0/3", "Arctic/Longyearbyen"},	
	{ LANG_ASIA_ADEN, "AST-3", "Asia/Aden"},	
	{ LANG_ASIA_ALMATY, "ALMT-6", "Asia/Almaty"},	
	{ LANG_ASIA_ANADYR, "ANAT-11ANAST,M3.5.0,M10.5.0/3", "Asia/Anadyr"},	
	{ LANG_ASIA_AQTAU, "AQTT-5", "Asia/Aqtau"},	
	{ LANG_ASIA_AQTOBE, "AQTT-5", "Asia/Aqtobe"},	
	{ LANG_ASIA_ASHGABAT, "TMT-5", "Asia/Ashgabat"},	
	{ LANG_ASIA_BAGHDAD, "AST-3", "Asia/Baghdad"},	
	{ LANG_ASIA_BAHRAIN, "AST-3", "Asia/Bahrain"},	
	{ LANG_ASIA_BAKU, "AZT-4AZST,M3.5.0/4,M10.5.0/5", "Asia/Baku"},	
	{ LANG_ASIA_BANGKOK, "ICT-7", "Asia/Bangkok"},	
	{ LANG_ASIA_BEIRUT, "EET-2EEST,M3.5.0/0,M10.5.0/0", "Asia/Beirut"},	
	{ LANG_ASIA_BISHKEK, "KGT-6", "Asia/Bishkek"},	
	{ LANG_ASIA_BRUNEI, "BNT-8", "Asia/Brunei"},	
	{ LANG_ASIA_CHOIBALSAN, "CHOT-8", "Asia/Choibalsan"},	
	{ LANG_ASIA_CHONGQING, "CST-8", "Asia/Chongqing"},	
	{ LANG_ASIA_COLOMBO, "IST-5:30", "Asia/Colombo"},	
	{ LANG_ASIA_DAMASCUS, "EET-2EEST,M4.1.5/0,M10.5.5/0", "Asia/Damascus"},	
	{ LANG_ASIA_DHAKA, "BDT-6", "Asia/Dhaka"},	
	{ LANG_ASIA_DILI, "TLT-9", "Asia/Dili"},	
	{ LANG_ASIA_DUBAI, "GST-4", "Asia/Dubai"},	
	{ LANG_ASIA_DUSHANBE, "TJT-5", "Asia/Dushanbe"},	
	{ LANG_ASIA_GAZA, "EET-2EEST,M3.5.6/0:01,M9.1.5", "Asia/Gaza"},	
	{ LANG_ASIA_HARBIN, "CST-8", "Asia/Harbin"},	
	{ LANG_ASIA_HO_CHI_MINH, "ICT-7", "Asia/Ho Chi Minh"},	
	{ LANG_ASIA_HONG_KONG, "HKT-8", "Asia/Hong Kong"},	
	{ LANG_ASIA_HOVD, "HOVT-7", "Asia/Hovd"},	
	{ LANG_ASIA_IRKUTSK, "IRKT-8IRKST,M3.5.0,M10.5.0/3", "Asia/Irkutsk"},	
	{ LANG_ASIA_JAKARTA, "WIT-7", "Asia/Jakarta"},	
	{ LANG_ASIA_JAYAPURA, "EIT-9", "Asia/Jayapura"},	
	{ LANG_ASIA_KABUL, "AFT-4:30", "Asia/Kabul"},	
	{ LANG_ASIA_KAMCHATKA, "PETT-11PETST,M3.5.0,M10.5.0/3", "Asia/Kamchatka"},	
	{ LANG_ASIA_KARACHI, "PKT-5", "Asia/Karachi"},	
	{ LANG_ASIA_KASHGAR, "CST-8", "Asia/Kashgar"},	
	{ LANG_ASIA_KATHMANDU, "NPT-5:45", "Asia/Kathmandu"},	
	{ LANG_ASIA_KOLKATA, "IST-5:30", "Asia/Kolkata"},	
	{ LANG_ASIA_KRASNOYARSK, "KRAT-7KRAST,M3.5.0,M10.5.0/3", "Asia/Krasnoyarsk"},	
	{ LANG_ASIA_KUALA_LUMPUR, "MYT-8", "Asia/Kuala Lumpur"},	
	{ LANG_ASIA_KUCHING, "MYT-8", "Asia/Kuching"},	
	{ LANG_ASIA_KUWAIT, "AST-3", "Asia/Kuwait"},	
	{ LANG_ASIA_MACAU, "CST-8", "Asia/Macau"},	
	{ LANG_ASIA_MAGADAN, "MAGT-11MAGST,M3.5.0,M10.5.0/3", "Asia/Magadan"},	
	{ LANG_ASIA_MAKASSAR, "CIT-8", "Asia/Makassar"},	
	{ LANG_ASIA_MANILA, "PHT-8", "Asia/Manila"},	
	{ LANG_ASIA_MUSCAT, "GST-4", "Asia/Muscat"},	
	{ LANG_ASIA_NICOSIA, "EET-2EEST,M3.5.0/3,M10.5.0/4", "Asia/Nicosia"},	
	{ LANG_ASIA_NOVOKUZNETSK, "NOVT-6NOVST,M3.5.0,M10.5.0/3", "Asia/Novokuznetsk"},	
	{ LANG_ASIA_NOVOSIBIRSK, "NOVT-6NOVST,M3.5.0,M10.5.0/3", "Asia/Novosibirsk"},	
	{ LANG_ASIA_OMSK, "OMST-7", "Asia/Omsk"},	
	{ LANG_ASIA_ORAL, "ORAT-5", "Asia/Oral"},	
	{ LANG_ASIA_PHNOM_PENH, "ICT-7", "Asia/Phnom Penh"},	
	{ LANG_ASIA_PONTIANAK, "WIT-7", "Asia/Pontianak"},	
	{ LANG_ASIA_PYONGYANG, "KST-9", "Asia/Pyongyang"},	
	{ LANG_ASIA_QATAR, "AST-3", "Asia/Qatar"},	
	{ LANG_ASIA_QYZYLORDA, "QYZT-6", "Asia/Qyzylorda"},	
	{ LANG_ASIA_RANGOON, "MMT-6:30", "Asia/Rangoon"},	
	{ LANG_ASIA_RIYADH, "AST-3", "Asia/Riyadh"},	
	{ LANG_ASIA_SAKHALIN, "SAKT-10SAKST,M3.5.0,M10.5.0/3", "Asia/Sakhalin"},	
	{ LANG_ASIA_SAMARKAND, "UZT-5", "Asia/Samarkand"},	
	{ LANG_ASIA_SEOUL, "KST-9", "Asia/Seoul"},	
	{ LANG_ASIA_SHANGHAI, "CST-8", "Asia/Shanghai"},	
	{ LANG_ASIA_SINGAPORE, "SGT-8", "Asia/Singapore"},	
	{ LANG_ASIA_TAIPEI, "CST-8", "Asia/Taipei"},	
	{ LANG_ASIA_TASHKENT, "UZT-5", "Asia/Tashkent"},	
	{ LANG_ASIA_TBILISI, "GET-4", "Asia/Tbilisi"},	
	{ LANG_ASIA_THIMPHU, "BTT-6", "Asia/Thimphu"},	
	{ LANG_ASIA_TOKYO, "JST-9", "Asia/Tokyo"},	
	{ LANG_ASIA_ULAANBAATAR, "ULAT-8", "Asia/Ulaanbaatar"},	
	{ LANG_ASIA_URUMQI, "CST-8", "Asia/Urumqi"},	
	{ LANG_ASIA_VIENTIANE, "ICT-7", "Asia/Vientiane"},	
	{ LANG_ASIA_VLADIVOSTOK, "VLAT-10VLAST,M3.5.0,M10.5.0/3", "Asia/Vladivostok"},	
	{ LANG_ASIA_YAKUTSK, "YAKT-9YAKST,M3.5.0,M10.5.0/3", "Asia/Yakutsk"},	
	{ LANG_ASIA_YEKATERINBURG, "YEKT-5YEKST,M3.5.0,M10.5.0/3", "Asia/Yekaterinburg"},	
	{ LANG_ASIA_YEREVAN, "AMT-4AMST,M3.5.0,M10.5.0/3", "Asia/Yerevan"},	
	{ LANG_ATLANTIC_AZORES, "AZOT1AZOST,M3.5.0/0,M10.5.0/1", "Atlantic/Azores"},	
	{ LANG_ATLANTIC_BERMUDA, "AST4ADT,M3.2.0,M11.1.0", "Atlantic/Bermuda"},	
	{ LANG_ATLANTIC_CANARY, "WET0WEST,M3.5.0/1,M10.5.0", "Atlantic/Canary"},	
	{ LANG_ATLANTIC_CAPE_VERDE, "CVT1", "Atlantic/Cape Verde"},	
	{ LANG_ATLANTIC_FAROE, "WET0WEST,M3.5.0/1,M10.5.0", "Atlantic/Faroe"},	
	{ LANG_ATLANTIC_MADEIRA, "WET0WEST,M3.5.0/1,M10.5.0", "Atlantic/Madeira"},	
	{ LANG_ATLANTIC_REYKJAVIK, "GMT0", "Atlantic/Reykjavik"},	
	{ LANG_ATLANTIC_SOUTH_GEORGIA, "GST2", "Atlantic/South Georgia"},	
	{ LANG_ATLANTIC_ST_HELENA, "GMT0", "Atlantic/St Helena"},	
	{ LANG_ATLANTIC_STANLEY, "FKT4FKST,M9.1.0,M4.3.0", "Atlantic/Stanley"},	
	{ LANG_AUSTRALIA_ADELAIDE, "CST-9:30CST", "Australia/Adelaide"},	
	{ LANG_AUSTRALIA_BRISBANE, "EST-10", "Australia/Brisbane"},	
	{ LANG_AUSTRALIA_BROKEN_HILL, "CST-9:30CST,M10.1.0,M4.1.0/3", "Australia/Broken Hill"},	
	{ LANG_AUSTRALIA_CURRIE, "EST-10EST,M10.1.0,M4.1.0/3", "Australia/Currie"},	
	{ LANG_AUSTRALIA_DARWIN, "CST-9:30", "Australia/Darwin"},	
	{ LANG_AUSTRALIA_EUCLA, "CWST-8:45", "Australia/Eucla"},	
	{ LANG_AUSTRALIA_HOBART, "EST-10EST,M10.1.0,M4.1.0/3", "Australia/Hobart"},	
	{ LANG_AUSTRALIA_LINDEMAN, "EST-10", "Australia/Lindeman"},	
	{ LANG_AUSTRALIA_LORD_HOWE, "LHST-10:30LHST-11,M10.1.0,M4.1.0 ", "Australia/Lord Howe"},	
	{ LANG_AUSTRALIA_MELBOURNE, "EST-10EST,M10.1.0,M4.1.0/3", "Australia/Melbourne"},	
	{ LANG_AUSTRALIA_PERTH, "WST-8", "Australia/Perth"},	
	{ LANG_AUSTRALIA_SYDNEY, "EST-10EST,M10.1.0,M4.1.0/3", "Australia/Sydney"},	
	{ LANG_EUROPE_AMSTERDAM, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Amsterdam"},	
	{ LANG_EUROPE_ANDORRA, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Andorra"},	
	{ LANG_EUROPE_ATHENS, "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europe/Athens"},	
	{ LANG_EUROPE_BELGRADE, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Belgrade"},	
	{ LANG_EUROPE_BERLIN, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Berlin"},	
	{ LANG_EUROPE_BRATISLAVA, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Bratislava"},	
	{ LANG_EUROPE_BRUSSELS, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Brussels"},	
	{ LANG_EUROPE_BUCHAREST, "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europe/Bucharest"},	
	{ LANG_EUROPE_BUDAPEST, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Budapest"},	
	{ LANG_EUROPE_CHISINAU, "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europe/Chisinau"},	
	{ LANG_EUROPE_COPENHAGEN, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Copenhagen"},	
	{ LANG_EUROPE_DUBLIN, "GMT0IST,M3.5.0/1,M10.5.0", "Europe/Dublin"},	
	{ LANG_EUROPE_GIBRALTAR, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Gibraltar"},	
	{ LANG_EUROPE_GUERNSEY, "GMT0BST,M3.5.0/1,M10.5.0", "Europe/Guernsey"},	
	{ LANG_EUROPE_HELSINKI, "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europe/Helsinki"},	
	{ LANG_EUROPE_ISLE_OF_MAN, "GMT0BST,M3.5.0/1,M10.5.0", "Europe/Isle of Man"},	
	{ LANG_EUROPE_ISTANBUL, "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europe/Istanbul"},	
	{ LANG_EUROPE_JERSEY, "GMT0BST,M3.5.0/1,M10.5.0", "Europe/Jersey"},	
	{ LANG_EUROPE_KALININGRAD, "EET-2EEST,M3.5.0,M10.5.0/3", "Europe/Kaliningrad"},	
	{ LANG_EUROPE_KIEV, "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europe/Kiev"},	
	{ LANG_EUROPE_LISBON, "WET0WEST,M3.5.0/1,M10.5.0", "Europe/Lisbon"},	
	{ LANG_EUROPE_LJUBLJANA, "WET0WEST,M3.5.0/1,M10.5.0", "Europe/Ljubljana"},	
	{ LANG_EUROPE_LONDON, "GMT0BST,M3.5.0/1,M10.5.0", "Europe/London"},	
	{ LANG_EUROPE_LUXEMBOURG, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Luxembourg"},	
	{ LANG_EUROPE_MADRID, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Madrid"},	
	{ LANG_EUROPE_MALTA, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Malta"},	
	{ LANG_EUROPE_MARIEHAMN, "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europe/Mariehamn"},	
	{ LANG_EUROPE_MINSK, "EET-2EEST,M3.5.0,M10.5.0/3", "Europe/Minsk"},	
	{ LANG_EUROPE_MONACO, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Monaco"},	
	{ LANG_EUROPE_MOSCOW, "MSK-3", "Europe/Moscow"},	//Change to -3 after 2014
	{ LANG_EUROPE_OSLO, "CET-1CEST,M3.5.0,M10.5.0/3 ", "Europe/Oslo"},	
	{ LANG_EUROPE_PARIS, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Paris"},	
	{ LANG_EUROPE_PODGORICA, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Podgorica"},	
	{ LANG_EUROPE_PRAGUE, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Prague"},	
	{ LANG_EUROPE_RIGA, "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europe/Riga"},	
	{ LANG_EUROPE_ROME, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Rome"},	
	{ LANG_EUROPE_SAMARA, "SAMT-3SAMST,M3.5.0,M10.5.0/3", "Europe/Samara"},	
	{ LANG_EUROPE_SAN_MARINO, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/San Marino"},	
	{ LANG_EUROPE_SARAJEVO, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Sarajevo"},	
	{ LANG_EUROPE_SIMFEROPOL, "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europe/Simferopol"},	
	{ LANG_EUROPE_SKOPJE, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Skopje"},	
	{ LANG_EUROPE_SOFIA, "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europe/Sofia"},	
	{ LANG_EUROPE_STOCKHOLM, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Stockholm"},	
	{ LANG_EUROPE_TALLINN, "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europe/Tallinn"},	
	{ LANG_EUROPE_TIRANE, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Tirane"},	
	{ LANG_EUROPE_UZHGOROD, "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europe/Uzhgorod"},	
	{ LANG_EUROPE_VADUZ, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Vaduz"},	
	{ LANG_EUROPE_VATICAN, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Vatican"},	
	{ LANG_EUROPE_VIENNA, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Vienna"},	
	{ LANG_EUROPE_VILNIUS, "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europe/Vilnius"},	
	{ LANG_EUROPE_VOLGOGRAD, "VOLT-3VOLST,M3.5.0,M10.5.0/3", "Europe/Volgograd"},	
	{ LANG_EUROPE_WARSAW, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Warsaw"},	
	{ LANG_EUROPE_ZAGREB, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Zagreb"},	
	{ LANG_EUROPE_ZAPOROZHYE, "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europe/Zaporozhye"},	
	{ LANG_EUROPE_ZURICH, "CET-1CEST,M3.5.0,M10.5.0/3", "Europe/Zurich"},	
	{ LANG_INDIAN_ANTANANARIVO, "EAT-3", "Indian/Antananarivo"},	
	{ LANG_INDIAN_CHAGOS, "IOT-6", "Indian/Chagos"},	
	{ LANG_INDIAN_CHRISTMAS, "CXT-7", "Indian/Christmas"},	
	{ LANG_INDIAN_COCOS, "CCT-6:30", "Indian/Cocos"},	
	{ LANG_INDIAN_COMORO, "EAT-3", "Indian/Comoro"},	
	{ LANG_INDIAN_KERGUELEN, "TFT-5", "Indian/Kerguelen"},	
	{ LANG_INDIAN_MAHE, "SCT-4", "Indian/Mahe"},	
	{ LANG_INDIAN_MALDIVES, "MVT-5", "Indian/Maldives"},	
	{ LANG_INDIAN_MAURITIUS, "MUT-4", "Indian/Mauritius"},	
	{ LANG_INDIAN_MAYOTTE, "EAT-3", "Indian/Mayotte"},	
	{ LANG_INDIAN_REUNION, "RET-4", "Indian/Reunion"},	
	{ LANG_PACIFIC_APIA, "WST11", "Pacific/Apia"},	
	{ LANG_PACIFIC_AUCKLAND, "NZST-12NZDT,M9.5.0,M4.1.0/3", "Pacific/Auckland"},	
	{ LANG_PACIFIC_CHATHAM, "CHAST-12:45CHADT,M9.5.0/2:45,M4.1.0/3:45", "Pacific/Chatham"},	
	{ LANG_PACIFIC_EFATE, "VUT-11", "Pacific/Efate"},	
	{ LANG_PACIFIC_ENDERBURY, "PHOT-13", "Pacific/Enderbury"},	
	{ LANG_PACIFIC_FAKAOFO, "TKT10", "Pacific/Fakaofo"},	
	{ LANG_PACIFIC_FIJI, "FJT-12", "Pacific/Fiji"},	
	{ LANG_PACIFIC_FUNAFUTI, "TVT-12", "Pacific/Funafuti"},	
	{ LANG_PACIFIC_GALAPAGOS, "GALT6", "Pacific/Galapagos"},	
	{ LANG_PACIFIC_GAMBIER, "GAMT9", "Pacific/Gambier"},	
	{ LANG_PACIFIC_GUADALCANAL, "SBT-11", "Pacific/Guadalcanal"},	
	{ LANG_PACIFIC_GUAM, "ChST-10", "Pacific/Guam"},	
	{ LANG_PACIFIC_HONOLULU, "HST10", "Pacific/Honolulu"},	
	{ LANG_PACIFIC_JOHNSTON, "HST10", "Pacific/Johnston"},	
	{ LANG_PACIFIC_KIRITIMATI, "LINT-14", "Pacific/Kiritimati"},	
	{ LANG_PACIFIC_KOSRAE, "KOST-11", "Pacific/Kosrae"},	
	{ LANG_PACIFIC_KWAJALEIN, "MHT-12", "Pacific/Kwajalein"},	
	{ LANG_PACIFIC_MAJURO, "MHT-12", "Pacific/Majuro"},	
	{ LANG_PACIFIC_MARQUESAS, "MART9:30", "Pacific/Marquesas"},	
	{ LANG_PACIFIC_MIDWAY, "SST11", "Pacific/Midway"},	
	{ LANG_PACIFIC_NAURU, "NRT-12", "Pacific/Nauru"},	
	{ LANG_PACIFIC_NIUE, "NUT11", "Pacific/Niue"},	
	{ LANG_PACIFIC_NORFOLK, "NFT-11:30", "Pacific/Norfolk"},	
	{ LANG_PACIFIC_NOUMEA, "NCT-11", "Pacific/Noumea"},	
	{ LANG_PACIFIC_PAGO_PAGO, "SST11", "Pacific/Pago Pago"},	
	{ LANG_PACIFIC_PALAU, "PWT-9", "Pacific/Palau"},	
	{ LANG_PACIFIC_PITCAIRN, "PST8", "Pacific/Pitcairn"},	
	{ LANG_PACIFIC_PONAPE, "PONT-11", "Pacific/Ponape"},	
	{ LANG_PACIFIC_PORT_MORESBY, "PGT-10", "Pacific/Port Moresby"},	
	{ LANG_PACIFIC_RAROTONGA, "CKT10", "Pacific/Rarotonga"},	
	{ LANG_PACIFIC_SAIPAN, "ChST-10", "Pacific/Saipan"},	
	{ LANG_PACIFIC_TAHITI, "TAHT10", "Pacific/Tahiti"},	
	{ LANG_PACIFIC_TARAWA, "GILT-12", "Pacific/Tarawa"},	
	{ LANG_PACIFIC_TONGATAPU, "TOT-13", "Pacific/Tongatapu"},	
	{ LANG_PACIFIC_TRUK, "TRUT-10", "Pacific/Truk"},	
	{ LANG_PACIFIC_WAKE, "WAKT-12", "Pacific/Wake"},	
	{ LANG_PACIFIC_WALLIS, "WFT-12", "Pacific/Wallis"},	
};

const size_t nr_tz = sizeof(tz_db) / sizeof(tz_db[0]);

const char *get_tz_utc_offset(unsigned int index)
{
	static char utc_offset[16];
	char *ret, *endptr, hour;
	unsigned char minute;

	if (index >= nr_tz)
		return "";

	ret = strpbrk(tz_db[index].string, "+-0123456789");
	if (ret) {
		hour = strtol(ret, &endptr, 10);
		minute = (endptr && *endptr == ':') ? strtoul(endptr + 1, NULL, 10) : 0;
	} else {
		hour = minute = 0;
	}

	snprintf(utc_offset, sizeof(utc_offset), "%+03hhd:%02hhu", -hour, minute);

	return utc_offset;
}

const char *get_tz_string(unsigned int index, unsigned char dst_enabled)
{
	static char tz_string[16];
	char *ret;

	if (index >= nr_tz)
		return "";

	if (dst_enabled) {
		return tz_db[index].string;
	} else {
		ret = strpbrk(tz_db[index].string, "+-0123456789");
		if (ret)
			ret = strpbrk(ret, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

		if (ret) {
			memset(tz_string, 0, sizeof(tz_string));
			strncpy(tz_string, tz_db[index].string, ret - tz_db[index].string);

			return tz_string;
		} else {
			/* No DST information in dst_string */
			return tz_db[index].string;
		}
	}
}

const char *get_tz_location(unsigned int index, tz_chk_t tz_chk)
{
	if (index >= nr_tz)
		return "";

	return (!tz_chk)?(multilang (tz_db[index].location)):(tz_db[index].location_cli);
}

const char *get_tz_location_cli(unsigned int index)
{
	if (index >= nr_tz)
		return "";

	return tz_db[index].location_cli;
}

#ifndef __UCLIBC__ //__GLIBC__
int is_tz_dst_exist(unsigned int index)
{
	char *ret;
	
	if (index >= nr_tz)
		return 0;
	
	ret = strpbrk(tz_db[index].string, "+-0123456789");
	
	if (ret) {
		ret = strpbrk(ret, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

		if (ret) 
			return 1;
	}

	return 0;
}

void format_tz_location(char * location)
{
	if (!location) {
		fprintf(stderr, "[Error] TZ location is Null!");
		return;
	}

	while(*location != 0)
	{
		if(*location == ' ')
			*location = '_';
		
		location++;
	}
	return;
}

const char *get_format_tz_utc_offset(unsigned int index)
{
	static char utc_offset[16];
	char *ret, *endptr, hour;
	unsigned char minute;

	if (index >= nr_tz)
		return "";

	ret = strpbrk(tz_db[index].string, "+-0123456789");
	if (ret) {
		hour = strtol(ret, &endptr, 10);
		minute = (endptr && *endptr == ':') ? strtoul(endptr + 1, NULL, 10) : 0;
	} else {
		hour = minute = 0;
	}
	
	if(minute)
		snprintf(utc_offset, sizeof(utc_offset), "%c%d:%d",hour>0?'+':'-',hour>0?hour:-hour,minute);
	else
		snprintf(utc_offset, sizeof(utc_offset), "%c%d",hour>0?'+':'-',hour>0?hour:-hour);
	
	return utc_offset;
}
#endif // of __GLIBC__
