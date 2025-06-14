//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/vstpresetkeys.h
// Created by  : Steinberg, 2006
// Description : VST Preset Keys
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/vsttypes.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
//------------------------------------------------------------------------
/** Predefined Preset Attributes */
//------------------------------------------------------------------------
namespace PresetAttributes
{
/**
 * \defgroup presetAttributes Predefined Preset Attributes
 */
/**@{*/
const CString kPlugInName	  = "PlugInName";		///< plug-in name
const CString kPlugInCategory = "PlugInCategory";	///< eg. "Fx|Dynamics", "Instrument", "Instrument|Synth"

const CString kInstrument	  = "MusicalInstrument";///< eg. instrument group (like 'Piano' or 'Piano|A. Piano')
const CString kStyle		  = "MusicalStyle";		///< eg. 'Pop', 'Jazz', 'Classic'
const CString kCharacter	  = "MusicalCharacter";	///< eg. instrument nature (like 'Soft' 'Dry' 'Acoustic')

const CString kStateType	  = "StateType";		///< Type of the given state see \ref StateType : Project / Default Preset or Normal Preset
const CString kFilePathStringType = "FilePathString";	///< Full file path string (if available) where the preset comes from (be sure to use a bigger string when asking for it (with 1024 characters))
const CString kName			  = "Name";				///< name of the preset
const CString kFileName		  = "FileName";			///< filename of the preset (including extension)
/**@}*/
};

//------------------------------------------------------------------------
/** Predefined StateType used for Key kStateType */
//------------------------------------------------------------------------
namespace StateType {
/**
 * \defgroup stateType Context of State Restoration 
 * used for PresetAttributes::kStateType in IStreamAttributes
 */
/**@{*/
/** the state is restored from a project loading or it is saved in a project */
const CString kProject = "Project";

/** the state is restored from a preset (marked as default) or the host wants to store a default
 * state of the plug-in */
const CString kDefault = "Default";

/** the state is restored from a track preset */
const CString kTrackPreset = "TrackPreset";

//------------------------------------------------------------------------
/**@}*/
}

//------------------------------------------------------------------------
/** Predefined Musical Instrument */
//------------------------------------------------------------------------
namespace MusicalInstrument
{
/**
 * \defgroup musicalInstrument Predefined Musical Instrument
 */
/**@{*/
const CString kAccordion			= "Accordion";
const CString kAccordionAccordion	= "Accordion|Accordion";
const CString kAccordionHarmonica	= "Accordion|Harmonica";
const CString kAccordionOther		= "Accordion|Other";

const CString kBass					= "Bass";
const CString kBassABass			= "Bass|A. Bass";
const CString kBassEBass			= "Bass|E. Bass";
const CString kBassSynthBass		= "Bass|Synth Bass";
const CString kBassOther			= "Bass|Other";

const CString kBrass				= "Brass";
const CString kBrassFrenchHorn		= "Brass|French Horn";
const CString kBrassTrumpet			= "Brass|Trumpet";
const CString kBrassTrombone		= "Brass|Trombone";
const CString kBrassTuba			= "Brass|Tuba";
const CString kBrassSection			= "Brass|Section";
const CString kBrassSynth			= "Brass|Synth";
const CString kBrassOther			= "Brass|Other";

const CString kChromaticPerc		= "Chromatic Perc";
const CString kChromaticPercBell	= "Chromatic Perc|Bell";
const CString kChromaticPercMallett	= "Chromatic Perc|Mallett";
const CString kChromaticPercWood	= "Chromatic Perc|Wood";
const CString kChromaticPercPercussion = "Chromatic Perc|Percussion";
const CString kChromaticPercTimpani	= "Chromatic Perc|Timpani";
const CString kChromaticPercOther	= "Chromatic Perc|Other";

const CString kDrumPerc				= "Drum&Perc";
const CString kDrumPercDrumsetGM	= "Drum&Perc|Drumset GM";
const CString kDrumPercDrumset		= "Drum&Perc|Drumset";
const CString kDrumPercDrumMenues	= "Drum&Perc|Drum Menues";
const CString kDrumPercBeats		= "Drum&Perc|Beats";
const CString kDrumPercPercussion	= "Drum&Perc|Percussion";
const CString kDrumPercKickDrum		= "Drum&Perc|Kick Drum";
const CString kDrumPercSnareDrum	= "Drum&Perc|Snare Drum";
const CString kDrumPercToms			= "Drum&Perc|Toms";
const CString kDrumPercHiHats		= "Drum&Perc|HiHats";
const CString kDrumPercCymbals		= "Drum&Perc|Cymbals";
const CString kDrumPercOther		= "Drum&Perc|Other";

const CString kEthnic				= "Ethnic";
const CString kEthnicAsian			= "Ethnic|Asian";
const CString kEthnicAfrican		= "Ethnic|African";
const CString kEthnicEuropean		= "Ethnic|European";
const CString kEthnicLatin			= "Ethnic|Latin";
const CString kEthnicAmerican		= "Ethnic|American";
const CString kEthnicAlien			= "Ethnic|Alien";
const CString kEthnicOther			= "Ethnic|Other";

const CString kGuitar				= "Guitar/Plucked";
const CString kGuitarAGuitar		= "Guitar/Plucked|A. Guitar";
const CString kGuitarEGuitar		= "Guitar/Plucked|E. Guitar";
const CString kGuitarHarp			= "Guitar/Plucked|Harp";
const CString kGuitarEthnic			= "Guitar/Plucked|Ethnic";
const CString kGuitarOther			= "Guitar/Plucked|Other";

const CString kKeyboard				= "Keyboard";
const CString kKeyboardClavi		= "Keyboard|Clavi";
const CString kKeyboardEPiano		= "Keyboard|E. Piano";
const CString kKeyboardHarpsichord	= "Keyboard|Harpsichord";
const CString kKeyboardOther		= "Keyboard|Other";

const CString kMusicalFX			= "Musical FX";
const CString kMusicalFXHitsStabs	= "Musical FX|Hits&Stabs";
const CString kMusicalFXMotion		= "Musical FX|Motion";
const CString kMusicalFXSweeps		= "Musical FX|Sweeps";
const CString kMusicalFXBeepsBlips	= "Musical FX|Beeps&Blips";
const CString kMusicalFXScratches	= "Musical FX|Scratches";
const CString kMusicalFXOther		= "Musical FX|Other";

const CString kOrgan				= "Organ";
const CString kOrganElectric		= "Organ|Electric";
const CString kOrganPipe			= "Organ|Pipe";
const CString kOrganOther			= "Organ|Other";

const CString kPiano				= "Piano";
const CString kPianoAPiano			= "Piano|A. Piano";
const CString kPianoEGrand			= "Piano|E. Grand";
const CString kPianoOther			= "Piano|Other";

const CString kSoundFX				= "Sound FX";
const CString kSoundFXNature		= "Sound FX|Nature";
const CString kSoundFXMechanical	= "Sound FX|Mechanical";
const CString kSoundFXSynthetic		= "Sound FX|Synthetic";
const CString kSoundFXOther			= "Sound FX|Other";

const CString kStrings				= "Strings";
const CString kStringsViolin		= "Strings|Violin";
const CString kStringsViola			= "Strings|Viola";
const CString kStringsCello			= "Strings|Cello";
const CString kStringsBass			= "Strings|Bass";
const CString kStringsSection		= "Strings|Section";
const CString kStringsSynth			= "Strings|Synth";
const CString kStringsOther			= "Strings|Other";

const CString kSynthLead			= "Synth Lead";
const CString kSynthLeadAnalog		= "Synth Lead|Analog";
const CString kSynthLeadDigital		= "Synth Lead|Digital";
const CString kSynthLeadArpeggio	= "Synth Lead|Arpeggio";
const CString kSynthLeadOther		= "Synth Lead|Other";

const CString kSynthPad				= "Synth Pad";
const CString kSynthPadSynthChoir	= "Synth Pad|Synth Choir";
const CString kSynthPadAnalog		= "Synth Pad|Analog";
const CString kSynthPadDigital		= "Synth Pad|Digital";
const CString kSynthPadMotion		= "Synth Pad|Motion";
const CString kSynthPadOther		= "Synth Pad|Other";

const CString kSynthComp			= "Synth Comp";
const CString kSynthCompAnalog		= "Synth Comp|Analog";
const CString kSynthCompDigital		= "Synth Comp|Digital";
const CString kSynthCompOther		= "Synth Comp|Other";

const CString kVocal				= "Vocal";
const CString kVocalLeadVocal		= "Vocal|Lead Vocal";
const CString kVocalAdlibs			= "Vocal|Adlibs";
const CString kVocalChoir			= "Vocal|Choir";
const CString kVocalSolo			= "Vocal|Solo";
const CString kVocalFX				= "Vocal|FX";
const CString kVocalSpoken			= "Vocal|Spoken";
const CString kVocalOther			= "Vocal|Other";

const CString kWoodwinds			= "Woodwinds";
const CString kWoodwindsEthnic		= "Woodwinds|Ethnic";
const CString kWoodwindsFlute		= "Woodwinds|Flute";
const CString kWoodwindsOboe		= "Woodwinds|Oboe";
const CString kWoodwindsEnglHorn	= "Woodwinds|Engl. Horn";
const CString kWoodwindsClarinet	= "Woodwinds|Clarinet";
const CString kWoodwindsSaxophone	= "Woodwinds|Saxophone";
const CString kWoodwindsBassoon		= "Woodwinds|Bassoon";
const CString kWoodwindsOther		= "Woodwinds|Other";
/**@}*/
};

//------------------------------------------------------------------------
/** Predefined Musical Style */
namespace MusicalStyle
{
/**
 * \defgroup musicalStyle Predefined Musical Style 
 */
/**@{*/
const CString kAlternativeIndie					= "Alternative/Indie";
const CString kAlternativeIndieGothRock			= "Alternative/Indie|Goth Rock";
const CString kAlternativeIndieGrunge			= "Alternative/Indie|Grunge";
const CString kAlternativeIndieNewWave			= "Alternative/Indie|New Wave";
const CString kAlternativeIndiePunk				= "Alternative/Indie|Punk";
const CString kAlternativeIndieCollegeRock		= "Alternative/Indie|College Rock";
const CString kAlternativeIndieDarkWave			= "Alternative/Indie|Dark Wave";
const CString kAlternativeIndieHardcore			= "Alternative/Indie|Hardcore";

const CString kAmbientChillOut					= "Ambient/ChillOut";
const CString kAmbientChillOutNewAgeMeditation	= "Ambient/ChillOut|New Age/Meditation";
const CString kAmbientChillOutDarkAmbient		= "Ambient/ChillOut|Dark Ambient";
const CString kAmbientChillOutDowntempo			= "Ambient/ChillOut|Downtempo";
const CString kAmbientChillOutLounge			= "Ambient/ChillOut|Lounge";

const CString kBlues							= "Blues";
const CString kBluesAcousticBlues				= "Blues|Acoustic Blues";
const CString kBluesCountryBlues				= "Blues|Country Blues";
const CString kBluesElectricBlues				= "Blues|Electric Blues";
const CString kBluesChicagoBlues				= "Blues|Chicago Blues";

const CString kClassical						= "Classical";
const CString kClassicalBaroque					= "Classical|Baroque";
const CString kClassicalChamberMusic			= "Classical|Chamber Music";
const CString kClassicalMedieval				= "Classical|Medieval";
const CString kClassicalModernComposition		= "Classical|Modern Composition";
const CString kClassicalOpera					= "Classical|Opera";
const CString kClassicalGregorian				= "Classical|Gregorian";
const CString kClassicalRenaissance				= "Classical|Renaissance";
const CString kClassicalClassic					= "Classical|Classic";
const CString kClassicalRomantic				= "Classical|Romantic";
const CString kClassicalSoundtrack				= "Classical|Soundtrack";

const CString kCountry							= "Country";
const CString kCountryCountryWestern			= "Country|Country/Western";
const CString kCountryHonkyTonk					= "Country|Honky Tonk";
const CString kCountryUrbanCowboy				= "Country|Urban Cowboy";
const CString kCountryBluegrass					= "Country|Bluegrass";
const CString kCountryAmericana					= "Country|Americana";
const CString kCountrySquaredance				= "Country|Squaredance";
const CString kCountryNorthAmericanFolk			= "Country|North American Folk";

const CString kElectronicaDance					= "Electronica/Dance";
const CString kElectronicaDanceMinimal			= "Electronica/Dance|Minimal";
const CString kElectronicaDanceClassicHouse		= "Electronica/Dance|Classic House";
const CString kElectronicaDanceElektroHouse		= "Electronica/Dance|Elektro House";
const CString kElectronicaDanceFunkyHouse		= "Electronica/Dance|Funky House";
const CString kElectronicaDanceIndustrial		= "Electronica/Dance|Industrial";
const CString kElectronicaDanceElectronicBodyMusic	= "Electronica/Dance|Electronic Body Music";
const CString kElectronicaDanceTripHop			= "Electronica/Dance|Trip Hop";
const CString kElectronicaDanceTechno			= "Electronica/Dance|Techno";
const CString kElectronicaDanceDrumNBassJungle	= "Electronica/Dance|Drum'n'Bass/Jungle";
const CString kElectronicaDanceElektro			= "Electronica/Dance|Elektro";
const CString kElectronicaDanceTrance			= "Electronica/Dance|Trance";
const CString kElectronicaDanceDub				= "Electronica/Dance|Dub";
const CString kElectronicaDanceBigBeats			= "Electronica/Dance|Big Beats";

const CString kExperimental						= "Experimental";
const CString kExperimentalNewMusic				= "Experimental|New Music";
const CString kExperimentalFreeImprovisation	= "Experimental|Free Improvisation";
const CString kExperimentalElectronicArtMusic	= "Experimental|Electronic Art Music";
const CString kExperimentalNoise				= "Experimental|Noise";

const CString kJazz								= "Jazz";
const CString kJazzNewOrleansJazz				= "Jazz|New Orleans Jazz";
const CString kJazzTraditionalJazz				= "Jazz|Traditional Jazz";
const CString kJazzOldtimeJazzDixiland			= "Jazz|Oldtime Jazz/Dixiland";
const CString kJazzFusion						= "Jazz|Fusion";
const CString kJazzAvantgarde					= "Jazz|Avantgarde";
const CString kJazzLatinJazz					= "Jazz|Latin Jazz";
const CString kJazzFreeJazz						= "Jazz|Free Jazz";
const CString kJazzRagtime						= "Jazz|Ragtime";

const CString kPop								= "Pop";
const CString kPopBritpop						= "Pop|Britpop";
const CString kPopRock							= "Pop|Pop/Rock";
const CString kPopTeenPop						= "Pop|Teen Pop";
const CString kPopChartDance					= "Pop|Chart Dance";
const CString kPop80sPop						= "Pop|80's Pop";
const CString kPopDancehall						= "Pop|Dancehall";
const CString kPopDisco							= "Pop|Disco";

const CString kRockMetal						= "Rock/Metal";
const CString kRockMetalBluesRock				= "Rock/Metal|Blues Rock";
const CString kRockMetalClassicRock				= "Rock/Metal|Classic Rock";
const CString kRockMetalHardRock				= "Rock/Metal|Hard Rock";
const CString kRockMetalRockRoll				= "Rock/Metal|Rock &amp; Roll";
const CString kRockMetalSingerSongwriter		= "Rock/Metal|Singer/Songwriter";
const CString kRockMetalHeavyMetal				= "Rock/Metal|Heavy Metal";
const CString kRockMetalDeathBlackMetal			= "Rock/Metal|Death/Black Metal";
const CString kRockMetalNuMetal					= "Rock/Metal|NuMetal";
const CString kRockMetalReggae					= "Rock/Metal|Reggae";
const CString kRockMetalBallad					= "Rock/Metal|Ballad";
const CString kRockMetalAlternativeRock			= "Rock/Metal|Alternative Rock";
const CString kRockMetalRockabilly				= "Rock/Metal|Rockabilly";
const CString kRockMetalThrashMetal				= "Rock/Metal|Thrash Metal";
const CString kRockMetalProgressiveRock			= "Rock/Metal|Progressive Rock";

const CString kUrbanHipHopRB					= "Urban (Hip-Hop / R&B)";
const CString kUrbanHipHopRBClassic 			= "Urban (Hip-Hop / R&B)|Classic R&B";
const CString kUrbanHipHopRBModern				= "Urban (Hip-Hop / R&B)|Modern R&B";
const CString kUrbanHipHopRBPop					= "Urban (Hip-Hop / R&B)|R&B Pop";
const CString kUrbanHipHopRBWestCoastHipHop		= "Urban (Hip-Hop / R&B)|WestCoast Hip-Hop";
const CString kUrbanHipHopRBEastCoastHipHop		= "Urban (Hip-Hop / R&B)|EastCoast Hip-Hop";
const CString kUrbanHipHopRBRapHipHop			= "Urban (Hip-Hop / R&B)|Rap/Hip Hop";
const CString kUrbanHipHopRBSoul				= "Urban (Hip-Hop / R&B)|Soul";
const CString kUrbanHipHopRBFunk				= "Urban (Hip-Hop / R&B)|Funk";

const CString kWorldEthnic						= "World/Ethnic";
const CString kWorldEthnicAfrica				= "World/Ethnic|Africa";
const CString kWorldEthnicAsia					= "World/Ethnic|Asia";
const CString kWorldEthnicCeltic				= "World/Ethnic|Celtic";
const CString kWorldEthnicEurope				= "World/Ethnic|Europe";
const CString kWorldEthnicKlezmer				= "World/Ethnic|Klezmer";
const CString kWorldEthnicScandinavia			= "World/Ethnic|Scandinavia";
const CString kWorldEthnicEasternEurope			= "World/Ethnic|Eastern Europe";
const CString kWorldEthnicIndiaOriental			= "World/Ethnic|India/Oriental";
const CString kWorldEthnicNorthAmerica			= "World/Ethnic|North America";
const CString kWorldEthnicSouthAmerica			= "World/Ethnic|South America";
const CString kWorldEthnicAustralia				= "World/Ethnic|Australia";
/**@}*/
};

//------------------------------------------------------------------------
/** Predefined Musical Character */
namespace MusicalCharacter
{
/**
\defgroup musicalCharacter Predefined Musical Character */
/**@{*/
//----TYPE------------------------------------
const CString kMono			= "Mono";
const CString kPoly			= "Poly";

const CString kSplit		= "Split";
const CString kLayer		= "Layer";

const CString kGlide		= "Glide";
const CString kGlissando	= "Glissando";

const CString kMajor		= "Major";
const CString kMinor		= "Minor";

const CString kSingle		= "Single";
const CString kEnsemble		= "Ensemble";

const CString kAcoustic		= "Acoustic";
const CString kElectric		= "Electric";

const CString kAnalog		= "Analog";
const CString kDigital		= "Digital";

const CString kVintage		= "Vintage";
const CString kModern		= "Modern";

const CString kOld			= "Old";
const CString kNew			= "New";

//----TONE------------------------------------
const CString kClean		= "Clean";
const CString kDistorted	= "Distorted";

const CString kDry			= "Dry";
const CString kProcessed	= "Processed";

const CString kHarmonic		= "Harmonic";
const CString kDissonant	= "Dissonant";

const CString kClear		= "Clear";
const CString kNoisy		= "Noisy";

const CString kThin			= "Thin";
const CString kRich			= "Rich";

const CString kDark			= "Dark";
const CString kBright		= "Bright";

const CString kCold			= "Cold";
const CString kWarm			= "Warm";

const CString kMetallic		= "Metallic";
const CString kWooden		= "Wooden";

const CString kGlass		= "Glass";
const CString kPlastic		= "Plastic";

//----ENVELOPE------------------------------------
const CString kPercussive	= "Percussive";
const CString kSoft			= "Soft";

const CString kFast			= "Fast";
const CString kSlow			= "Slow";

const CString kShort		= "Short";
const CString kLong			= "Long";

const CString kAttack		= "Attack";
const CString kRelease		= "Release";

const CString kDecay		= "Decay";
const CString kSustain		= "Sustain";

const CString kFastAttack	= "Fast Attack";
const CString kSlowAttack	= "Slow Attack";

const CString kShortRelease	= "Short Release";
const CString kLongRelease	= "Long Release";

const CString kStatic		= "Static";
const CString kMoving		= "Moving";

const CString kLoop			= "Loop";
const CString kOneShot		= "One Shot";
/**@}*/
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
