/*
 * Copyright (c) 2024 Sean Ditny
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * The Synthstrom Audible Deluge Firmware is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "definitions_cxx.hpp"
#include "gui/l10n/strings.h"
#include "processing/sound/sound_drum.h"
#include "util/d_string.h"
#include <cstdint>

class Output;
class Clip;
class Drum;
class Kit;
class SoundDrum;

class StemExport {
public:
	StemExport();

	// start & stop process
	void startStemExportProcess(StemExportType stemExportType);
	void stopStemExportProcess();
	void abortStemExportProcess(deluge::l10n::String reason);
	void startOutputRecordingUntilLoopEndAndSilence();
	void stopPlayback();
	void stopOutputRecording();
	bool checkForLoopEnd();
	bool checkForSilence();
	bool processStarted;
	bool stopRecording;
	StemExportType currentStemExportType;
	uint32_t timePlaybackStopped;
	uint32_t timeThereWasLastSomeActivity;

	// export config variables
	bool allowNormalization;
	bool allowNormalizationForDrums;
	bool exportToSilence;
	bool includeSongFX;
	bool includeKitFX;
	bool renderOffline;
	bool exportMixdown;
	/// arranger only: export each kit as one stem per drum row instead of one stem per kit
	bool splitKitsIntoDrums;
	/// split kits only: drum stems keep the kit affect-entire FX (includeKitFX stays the setting of the DRUM export)
	bool splitKitsIncludeKitFX;
	/// split kits only: stems include the reverb return of what they contain, without the other song FX
	bool splitKitsIncludeReverb;

	// split kits (arranger TRACK export with splitKitsIntoDrums enabled)
	/// latched when the export starts, so a menu change can never desync the save and the restore of row mutes
	bool splitKitsActive;
	/// kit whose drum is being exported in the current pass, nullptr outside a split kit pass
	Kit* currentSplitKit;
	bool isDrumEligibleForSplitExport(Kit* kit, Drum* drum);
	int32_t countDrumStemsForSplitKit(Kit* kit);
	void saveAllKitRowMutesForSplitExport();
	void restoreAllKitRowMutesAfterSplitExport();
	void muteAllKitRowsExceptDrum(Kit* kit, Drum* drum);
	void exportSplitKitDrumStems(StemExportType stemExportType, Kit* kit, int32_t idxOutput);
	void cutAllSoundBeforeNextStem();
	/// split kits with REVERB on and SONG FX off: stems are recorded as each element's dry signal plus its reverb
	/// return, without anything from the song level. Offline rendering only: when rendering live, stems are recorded
	/// as before.
	bool renderingDryWithReverb() const {
		return processStarted && splitKitsActive && splitKitsIncludeReverb && renderOffline && !includeSongFX;
	}
	void clearReverb();
	/// export to silence makes every stem a different length, so it is ignored when splitting kits
	bool shouldExportToSilence() const { return exportToSilence && !splitKitsActive; }
	/// whether the kit affect-entire FX must be bypassed when rendering this kit
	bool shouldBypassKitFX(const Kit* kit) const {
		if (!processStarted) {
			return false;
		}
		if (currentStemExportType == StemExportType::DRUM) {
			return !includeKitFX;
		}
		return currentSplitKit != nullptr && currentSplitKit == kit && !splitKitsIncludeKitFX;
	}

	// export instruments
	int32_t disarmAllInstrumentsForStemExport(StemExportType stemExportType);
	int32_t exportInstrumentStems(StemExportType stemExportType);
	int32_t exportMixdownStem(StemExportType stemExportType);
	void restoreAllInstrumentMutes(int32_t totalNumOutputs);

	// export clips
	int32_t disarmAllClipsForStemExport();
	int32_t exportClipStems(StemExportType stemExportType);
	void restoreAllClipMutes(int32_t totalNumClips);
	void getLoopLengthOfLongestNotEmptyNoteRow(Clip* clip);
	void getLoopEndPointInSamplesForAudioFile(int32_t loopLength);
	bool writeLoopEndPos();
	int32_t loopLengthToStopStemExport;
	int32_t loopEndPointInSamplesForAudioFile;

	// export drums
	int32_t disarmAllDrumsForStemExport();
	int32_t exportDrumStems(StemExportType stemExportType);
	void restoreAllDrumMutes(int32_t totalNumNoteRows);

	// start exporting
	bool startCurrentStemExport(StemExportType stemExportType, Output* output, bool& muteState, int32_t fileNumber,
	                            bool exportStem, SoundDrum* drum = nullptr, int32_t drumNumber = 0);

	// finish exporting
	void finishCurrentStemExport(StemExportType stemExportType, bool& muteState);
	void finishStemExportProcess(StemExportType stemExportType, int32_t elementsProcessed);
	void updateScrollPosition(StemExportType stemExportType, int32_t indexNumber);

	// export status
	void displayStemExportProgress(StemExportType stemExportType);
	void displayStemExportProgressOLED(StemExportType stemExportType);
	void displayStemExportProgress7SEG();
	int32_t numStemsExported;
	int32_t totalNumStemsToExport;

	// audio file management
	Error getUnusedStemRecordingFilePath(String* filePath, AudioRecordingFolder folder);
	Error getUnusedStemRecordingFolderPath(String* filePath, AudioRecordingFolder folder);
	int32_t highestUsedStemFolderNumber;
	String lastFolderNameForStemExport;
	/// returns false if no valid file name could be built (e.g. the output / drum names are too long
	/// for the file system), in which case the stem must not be exported
	[[nodiscard]] bool setWavFileNameForStemExport(StemExportType type, Output* output, int32_t fileNumber,
	                                               SoundDrum* drum = nullptr, int32_t drumNumber = 0);
	String wavFileNameForStemExport;
	bool wavFileNameForStemExportSet;

	// check if we're in context menu
	bool inContextMenu();
	bool renderingOffline() { return processStarted && renderOffline; }
};

extern StemExport stemExport;
