/*
 * open_al_test
 *
 * Copyright (c) 2000 - 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define debug_log(msg, args...) fprintf(stderr, msg, ##args)
#define debug_error(msg, args...) fprintf(stderr, msg, ##args)
#define MAX_STRING_LEN 256
#define MAX_PATH_LEN		1024
#define MIN_TONE_PLAY_TIME 300

#include <glib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>


#define POWERON_FILE	"/usr/share/feedback/sound/operation/power_on.wav"
#define KEYTONE_FILE	"/opt/usr/share/settings/Previews/touch.wav"

#define MMSOUND_STRNCPY strncpy

#define MM_ERROR_NONE 0

unsigned int g_volume_value;

GIOChannel *stdin_channel;

GMainLoop* g_loop;

// Function
static void interpret (char *buf);
gboolean timeout_menu_display();
gboolean timeout_quit_program();
gboolean input (GIOChannel *channel);
void printMenu();

void quit_program()
{
	debug_log("Quit the program\n");
	g_main_loop_quit(g_loop);
}

gboolean timeout_menu_display()
{
	printMenu();
	return FALSE;
}

gboolean timeout_quit_program()
{
	quit_program();
	return FALSE;
}

gboolean input (GIOChannel *channel)
{
    char *read_line = NULL;
    gsize read;
    gsize terminator;
    GError *error = NULL;

    g_io_channel_read_line(channel, &read_line, &read, &terminator, &error);
    read_line[terminator] = '\0';
    g_strstrip(read_line);
    interpret (read_line);
    g_free(read_line);
    return TRUE;
}

void
play_openAL(void)
{
	ALenum openAlResult = AL_NO_ERROR;

	ALsizei size, freq;
	ALfloat freq_float;
	ALenum format;
	ALvoid *data;
	//ALboolean loop = AL_FALSE;

	ALuint buffer;
	ALint source_state;

	ALuint source;

	alutInit(0,0);
	openAlResult = alGetError();
	if (openAlResult != AL_NO_ERROR) {
		g_print("Error alutInit Failed");
		return;
	}
	g_print("Playing file POWERON_FILE\n");
	/*Generate Buffers*/
	alGenBuffers((ALuint)1, &buffer);

	/*Get Wav file info*/
	data = alutLoadMemoryFromFile(POWERON_FILE, &format, &size, &freq_float);


	g_print("Data - %p Format - %d\t Size - %d, freq_float - %f\n", data, format, size, freq_float);

	freq = freq_float;

	/*Load buffer data*/
	alBufferData(buffer, format, data, size, freq);


	alGenSources((ALuint)1, &source);
	openAlResult = alGetError();
	if (openAlResult != AL_NO_ERROR) {
		g_print("Error alutInit Failed");
		return;
	}

	alSourcei(source, AL_BUFFER, buffer);

	alSourcef(source, AL_PITCH, 1);
	openAlResult = alGetError();
	if (openAlResult != AL_NO_ERROR) {
		g_print("Error alutInit Failed");
		return;
	}
	alSourcef(source, AL_GAIN, 1);
	openAlResult = alGetError();
	if (openAlResult != AL_NO_ERROR) {
		g_print("Error alutInit Failed");
		return;
	}
	alSource3f(source, AL_POSITION, 0, 0, 0);
	openAlResult = alGetError();
	if (openAlResult != AL_NO_ERROR) {
		g_print("Error alutInit Failed");
		return;
	}
	alSource3f(source, AL_VELOCITY, 0, 0, 0);
	openAlResult = alGetError();
	if (openAlResult != AL_NO_ERROR) {
		g_print("Error alutInit Failed");
		return;
	}
	alSourcei(source, AL_LOOPING, AL_FALSE);
	openAlResult = alGetError();
	if (openAlResult != AL_NO_ERROR) {
		g_print("Error alutInit Failed");
		return;
	}

	alSourcePlay(source);
	// check for errors

	alGetSourcei(source, AL_SOURCE_STATE, &source_state);
	// check for errors
	while (source_state == AL_PLAYING) {
		alGetSourcei(source, AL_SOURCE_STATE, &source_state);
		if (openAlResult != AL_NO_ERROR) {
			g_print("Error alutInit Failed");
			return;
		}
	}

	alDeleteSources(1, &source);
	alDeleteBuffers(1, &buffer);
	g_print("Done!!\n");
	return;
}

void printMenu()
{
	g_print("Pres 'p' to play 'x' to quit '?' for help\n");
}

static void interpret (char *cmd)
{
	switch(cmd[0])
	{
	case '?':
		printMenu();
		break;
	case 'p':
		play_openAL();
		break;
	case 'x':
		quit_program();
		break;
	default:
		g_print("Can't handle the command\n press ? for help\n");
		break;
	}

}


GData *func_prt_list;
int main()
{
	stdin_channel = g_io_channel_unix_new(0);
	g_io_add_watch(stdin_channel, G_IO_IN, (GIOFunc)input, NULL);
	g_loop = g_main_loop_new (NULL, 1);
	printMenu();
	g_main_loop_run (g_loop);
	return 0;
}


