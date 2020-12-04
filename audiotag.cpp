#include <iostream>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "fileio.hh"

#define ID3_2_MAX_FRAME_SIZE 60
#define ID3_1_FRAME_SIZE 30

// Flips endianess of 32-bit integer
uint32_t flip_endianness(uint32_t x) {
    uint32_t ret = 0;
    ret |= (x & 0xFF) << 24;
    ret |= (x & 0xFF00) << 8;
    ret |= (x & 0xFF000000) >> 24;
    ret |= (x & 0xFF0000) >> 8;
    return ret;
}

/**********************************************
 *  Required fields:
 *  These defines cause the program to prompt
 *  for any fields that have not been included
 *  in the frame already. You can comment out
 *  fields which are not needed
 **********************************************/
#define TITLE_REQUIRED
#define ARTIST_REQUIRED
#define ALBUM_REQUIRED
#define YEAR_REQUIRED
#define TRACK_REQUIRED
#define COMPOSER_REQUIRED

typedef enum {
#ifdef TITLE_REQUIRED
    title_idx, 
#endif

#ifdef ARTIST_REQUIRED
    artist_idx,
#endif

#ifdef ALBUM_REQUIRED
    album_idx,
#endif

#ifdef YEAR_REQUIRED
    year_idx,
#endif

#ifdef TRACK_REQUIRED
    track_idx,
#endif

#ifdef COMPOSER_REQUIRED
    composer_idx,
#endif
    NUM_TRAITS,
} trait_index_t;

static uint8_t traits[NUM_TRAITS];


/**********************************************
 *  id3v1 is the following format: 
 *  "TAG" - 3 bytes
 *  TITLE - 30 bytes
 *  ARTIST - 30 bytes
 *  ALBUM - 30 bytes
 *  YEAR - 4 bytes
 *  COMMENT/TRACK - 30 bytes 
 *  GENRE - 1 byte
 * https://id3.org/ID3v1
 **********************************************/
typedef struct id3_1_t {
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    char comment[30];
    char genre[1];
} id3_1_t;

/**********************************************
 *  id3v2 frames have the following format: 
 *  ID - 4 bytes
 *  SIZE - 4 bytes (big-endian)
 *  FLAGS - 2 bytes
 *  https://id3.org/id3v2.3.0
 **********************************************/
typedef struct id3_2_frame_header_t {
    char id[4];
    uint32_t size;
    uint32_t flags;
} id3_2_frame_t;


/**********************************************
 *  string_from_id: 
 *  given an ID3v2 frame ID puts a plaintext
 *  string of the frame content in s.
 *  Assumes that s is at least 8 bytes.
 *  If the ID is not recognized, the ID is
 *  placed in s.
 **********************************************/
void string_from_id(char* id, char* s) {
    if (strncmp(id, "TALB", 4) == 0) {
        memcpy(s, "Album", 5);
    } else if (strncmp(id, "TIT2", 4) == 0) {
        memcpy(s, "Title", 5);
    } else if (strncmp(id, "TORY", 4) == 0 || strncmp(id, "TYER", 4) == 0) {
        memcpy(s, "Year", 4);
    } else if (strncmp(id, "TPE1", 4) == 0) {
        memcpy(s, "Artist", 6);
    } else if (strncmp(id, "TRCK", 4) == 0) {
        memcpy(s, "Track", 5);
    } else if (strncmp(id, "TCOM", 4) == 0) {
        memcpy(s, "Composer", 8);
    } else {
        // Default to original id
        memcpy(s, id, 4);
    }
}

/**********************************************
 *  set_trait_from_tag_id: 
 *      given a required ID3v2 frame ID and a 
 *  value, marks that ID as value in the trait
 * array. Does nothing if not a required ID.
 **********************************************/
void set_trait_from_tag_id(char* id, int val) {
#ifdef ALBUM_REQUIRED
    if (strncmp(id, "TALB", 4) == 0) {
        traits[album_idx] = val;
    }
#endif

#ifdef TITLE_REQUIRED
    if (strncmp(id, "TIT2", 4) == 0) {
        traits[title_idx] = val;
    }
#endif

#ifdef YEAR_REQUIRED
    if (strncmp(id, "TORY", 4) == 0 || strncmp(id, "TYER", 4) == 0) {
        traits[year_idx] = val;
    }
#endif 

#ifdef ARTIST_REQUIRED
    if (strncmp(id, "TPE1", 4) == 0) {
        traits[artist_idx] = val;
    }
#endif

#ifdef TRACK_REQUIRED
    if (strncmp(id, "TRCK", 4) == 0) {
        traits[track_idx] = val;
    }
#endif 

#ifdef COMPOSER_REQUIRED
    if (strncmp(id, "TCOM", 4) == 0) {
        traits[composer_idx] = val;
    }
#endif
}

void remove_trait(char* id) {
    set_trait_from_tag_id(id, 0);
}

void add_trait(char* id) {
    set_trait_from_tag_id(id, 1);
}


/**********************************************
 *  prompt_input: 
 *      prompts for input by displaying the
 *  current text (current_text), a size limit
 *  (input_sz), and places the user input
 *  in prompt_input. Assumes that prompt_input
 *  is at least input_sz bytes large.
 **********************************************/
int prompt_input(char* prompt_text, char* current_text, char* prompt_input, size_t input_sz) {
    printf("%s: %.30s\n", prompt_text, current_text);
    char in = 0;
    while (in != 'n' && in != 'y') {
        printf("Update %s? (y/n) ", prompt_text);
        std::cin >> in;
    }
    std::cin.ignore();
    if (in == 'y') {
        printf("New %s (max %lu chars): ", prompt_text, input_sz);
        std::cin.getline(prompt_input, input_sz + 1);
    }
    printf("\n");
    return in == 'y';
}

/**********************************************
 *  get_id3_2_header:
 *    reads 10 bytes and places them in a
 *  frame_header struct.
 **********************************************/
id3_2_frame_header_t get_id3_2_header(int fd) {
    id3_2_frame_header_t frame; 

    // Read the frame
    int ret = read(fd, &frame, 10);
    frame.size = flip_endianness(frame.size);

    return frame;
}

/**********************************************
 *  add_id3_2_frame:
 *    adds an ID3 frame id with field text as 
 *  text.
 **********************************************/
void add_id3_2_frame(int fd, char* id, char* text, char* path) {

    long unsigned n = strlen(text);
    char* frame = (char*) malloc(n + 11); // 10 for header, n for text, 1 for encoding byte
    memcpy(frame, id, 4);
    uint32_t sz = flip_endianness(n + 1);
    memcpy(frame + 4, &sz, 4);
    memset(frame + 8, 0, 3);
    memcpy(frame + 11, text, n);

    add_bytes(fd, n + 11, (uint8_t*) frame, path);
    free(frame);

    return;
}

/**********************************************
 *  interpret_frame_text:
 *    Given a buffer from an ID3 frame, 
 *  interpret the string
 **********************************************/
void interpret_frame_text(char* buf, size_t sz) {
    char encoding_byte = buf[0];
    if (sz <= 1) {
        printf("VOID\n");
        return;
    }
    switch(encoding_byte) {
        case 0:
            // Ascii terminated with 0 byte
            for (int i = 1; i < sz; ++i) {
                printf("%c", buf[i]);
            }
            printf("\n");
            printf("Text Encoding: ASCII\n");
            break;
        case 1:
            // UTF-16 terminated with aligned double 0 byte
            assert((uint8_t) buf[1] == 0xff);
            assert((uint8_t) buf[2] == 0xfe);
            for (int i = 3; i < sz; i+=2) {
                printf("%c", buf[i]);
            }
            printf("\n");
            printf("Text Encoding: UTF-16 (LE)\n");
            break;
        case 2:
            assert((uint8_t) buf[1] == 0xfe);
            assert((uint8_t) buf[2] == 0xff);
            // UTF-16 (BE) terminated with aligned double 0 byte
            printf("Text Encoding: UTF-16 (BE)\n");
            break;
        case 3:
            // UTF-8 terminated with 0 byte
            printf("Text Encoding: UTF-8\n");
            break;

    }
}


/**********************************************
 *  handle_id3v2:
 *    parses for ID3v2 frames and prompts
 *  user for frame modifications.
 *  assumes that fd points to the first frame
 **********************************************/
void handle_id3v2(int fd, char* path, uint32_t tag_sz) {
    memset(traits, 0, sizeof(traits));
    // Get the first frame header
    id3_2_frame_header_t frame_header = get_id3_2_header(fd);

    int32_t added_bytes = 0;
    uint32_t bytes_read = 10;
    
    char field_text[ID3_2_MAX_FRAME_SIZE + 1];
    char field_plain_text[10];
    char zeroes[4] = {0,0,0,0};

    // While we haven't reached the end of the tag or hit padding
    while (bytes_read <= tag_sz + added_bytes && memcmp(frame_header.id, zeroes, 4) != 0) {

        memset(field_plain_text, 0, sizeof(field_plain_text));

        // Print the frame tag
        string_from_id(frame_header.id, (char*) field_plain_text);
        printf("%s (%d): ", field_plain_text, frame_header.size);

        // Alloc for the frame text
        char* frame_text = (char*) malloc(frame_header.size);
        if (frame_text == nullptr) {
            break;
        }

        // Read and interpret the text
        read(fd, frame_text, frame_header.size);
        bytes_read += frame_header.size;
        interpret_frame_text(frame_text, frame_header.size);

        add_trait(frame_header.id);

        char in = 0;
        while (in != 'y' && in != 'n') {
            std::cout << "Change field? (y/n): ";
            std::cin >> in;
        }

        while (in != 'n') {
            std::cout << "Remove field? (y/n): ";
            std::cin >> in;
            if (in == 'y') {
                off_t curr = lseek(fd, lseek(fd, 0, SEEK_CUR) - frame_header.size - 10, SEEK_SET);
                remove_bytes(fd, 10 + frame_header.size, path);
                remove_trait(frame_header.id);
                added_bytes -= frame_header.size;
                in = 'n';
            }
            else if (in == 'n') {
                in = 'y';
                break;
            }
        }

        std::cin.ignore();
    
        if (in == 'y') {
            std::cout << "New Text (max 60 chars): ";
            std::cin.getline(field_text, sizeof(field_text));

            off_t curr = lseek(fd, lseek(fd, 0, SEEK_CUR) - frame_header.size - 10, SEEK_SET);
            assert(curr != -1);

            unsigned long n = strlen(field_text);
            added_bytes += n - frame_header.size;

            remove_bytes(fd, 10 + frame_header.size, path);
            add_id3_2_frame(fd, frame_header.id, field_text, path);

            add_trait(frame_header.id);
        }
        std::cout << "\n";

        // Free the text buffer and get the next frame
        free(frame_text);
        in = 0;

        frame_header = get_id3_2_header(fd);
        bytes_read += 10;
    }
    // last frame was not an actual frame header, so backup
    lseek(fd, lseek(fd,0,SEEK_CUR) - 10, SEEK_SET);


    // Prompt for required traits
    for (int i = 0; i < NUM_TRAITS; ++i) {
        if (!traits[i]) {
            char in = 0;
            switch (i) {
                #ifdef ALBUM_REQUIRED
                case album_idx:
                    if (prompt_input((char*) "Album", (char*) "", field_text, ID3_2_MAX_FRAME_SIZE)) {
                        add_id3_2_frame(fd, (char*) "TALB", field_text, path);
                    }
                    break;
                #endif

                #ifdef COMPOSER_REQUIRED
                case composer_idx:
                    if (prompt_input((char*) "Composer",(char*) "", field_text, ID3_2_MAX_FRAME_SIZE)) {
                        add_id3_2_frame(fd, (char*) "TCOM", field_text, path);
                    }
                    break;
                #endif

                #ifdef YEAR_REQUIRED
                case year_idx:
                    if (prompt_input((char*) "Year", (char*)"", field_text, ID3_2_MAX_FRAME_SIZE)) {
                        add_id3_2_frame(fd, (char*) "TORY", field_text, path);
                    }
                    break;
                #endif

                #ifdef ARTIST_REQUIRED
                case artist_idx:
                    if (prompt_input((char*) "Artist",(char*) "", field_text, ID3_2_MAX_FRAME_SIZE)) {
                        add_id3_2_frame(fd, (char*) "TPE1", field_text, path);
                    }
                    break;
                #endif

                #ifdef TRACK_REQUIRED
                case track_idx:
                    if (prompt_input((char*) "Track", (char*)"", field_text, ID3_2_MAX_FRAME_SIZE)) {
                        add_id3_2_frame(fd, (char*) "TRCK", field_text, path);
                    }
                    break;
                #endif

                #ifdef TITLE_REQUIRED
                case title_idx:
                    if (prompt_input((char*) "Title", (char*)"", field_text, ID3_2_MAX_FRAME_SIZE)) {
                        add_id3_2_frame(fd, (char*) "TIT2", field_text, path);
                    }
                    break;
                #endif
                default:
                    break;
            }
        }
    }
}

/**********************************************
 *  handle_id3v1:
 *    parses an ID3v1 tag and prompts
 *  user for modifications.
 *  assumes that fd points to the title
 **********************************************/
void handle_id3v1(int fd) {
    off_t frame_start = lseek(fd, 0, SEEK_CUR);
    id3_1_t audio_tag;
    char new_text[ID3_1_FRAME_SIZE + 1];      // 30 bytes for content + 1 for newline
    memset(new_text, 0, sizeof(new_text));

    // Update Title
    read(fd, audio_tag.title, 30);
    if (prompt_input((char*) "Title", audio_tag.title, new_text, ID3_1_FRAME_SIZE)) {
        lseek(fd, frame_start, SEEK_SET);
        write(fd, new_text, 30);
        memset(new_text,0, sizeof(new_text));
    }

    // Update Artist
    read(fd, audio_tag.artist, 30);
    if (prompt_input((char*) "Artist", audio_tag.artist, new_text, ID3_1_FRAME_SIZE)) {
        lseek(fd, frame_start + 30, SEEK_SET);
        write(fd, new_text, 30);
        memset(new_text,0, sizeof(new_text));
    }

    // Update Album
    read(fd, audio_tag.album, 30);
    if (prompt_input((char*) "Album", audio_tag.album, new_text, ID3_1_FRAME_SIZE)) {
        lseek(fd, frame_start + 60, SEEK_SET);
        write(fd, new_text, 30);
        memset(new_text,0, sizeof(new_text));
    }

    // Update Year
    read(fd, audio_tag.year, 4);
    if (prompt_input((char*) "Year", audio_tag.year, new_text, 4)) {
        lseek(fd, frame_start + 90, SEEK_SET);
        write(fd, new_text, 4);
        memset(new_text,0, sizeof(new_text));
    }

    // Update Comment
    read(fd, audio_tag.comment, 30);
    if (audio_tag.comment[28] == 0) {
        
        // Temp buffer for track number
        char track_arr[3];
        uint8_t comment_modified = prompt_input((char*) "Comment", audio_tag.comment, new_text, 28);
        sprintf(track_arr, "%d", audio_tag.comment[29]);
        uint8_t track_modified = prompt_input((char*) "Track", track_arr, track_arr, 3);

        uint8_t track_num = atoi(track_arr);


        // If the comment is new
        if (comment_modified) {
            lseek(fd, frame_start + 94, SEEK_SET);
            // If the track is also new
            if (track_modified) {
                // Zero-byte
                new_text[28] = 0; 
                new_text[29] = track_num;
                write(fd, new_text, 30);
            } else {
                write(fd, new_text, 28);
                lseek(fd, 2, SEEK_CUR);
            }
        } else if (track_modified) {
            lseek(fd, frame_start + 94 + 29, SEEK_SET);
            write(fd, (char*) &track_num, 1);
        }
    }
    else {
        if (prompt_input((char*) "Comment", audio_tag.comment, new_text, ID3_1_FRAME_SIZE)) {
            lseek(fd, frame_start + 94, SEEK_SET);
            write(fd, new_text, 30);
        }
    }

    read(fd, audio_tag.genre, 1);
    sprintf(new_text, "%d", audio_tag.genre[0]);
    if (prompt_input((char*) "Genre ID", new_text, new_text, 3)) {
        uint8_t genre_num = atoi(new_text);
        lseek(fd, frame_start + 124, SEEK_SET);
        write(fd, (char*) &genre_num,1);
    }
}


int main(int argc, char* argv[]) {

    // Check arguments
    if (argc != 2) {
        std::cerr << "Usage: ./audiotag [file.mp3]\n";
        return 1;
    }

    // Check file is an mp3 file
    int n = strlen(argv[argc - 1]);
    if (memcmp(&argv[argc - 1][n - 4], ".mp3", 3) != 0) {
        std::cerr << "Usage: ./audiotag [file.mp3]\n";
        return 1;
    }

    // Open the file
    int fd = open(argv[argc - 1], O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        std::cerr << "Could not open " << argv[argc - 1] << "\n";
        return 2;
    }

    // Read the first ten bytes
    char tag_bytes[10];
    memset(tag_bytes, 0, 10);
    read(fd, tag_bytes, 10);

    // Check if we found an ID3.2 tag
    if (!memcmp("ID3", tag_bytes, 3)) {
        printf("ID3v2\n");
        uint32_t sz = (tag_bytes[6] << 21) | (tag_bytes[7] << 14) | (tag_bytes[8] << 7) | tag_bytes[9];

        handle_id3v2(fd, argv[argc - 1], sz);
    } else {
        // Otherwise check for an ID3v1 tag
        off_t end = lseek(fd, 0, SEEK_END);

        // ID3v1 tags start at end - 128
        off_t tag_start = end - 128;
        lseek(fd, tag_start, SEEK_SET);

        int res = read(fd, tag_bytes, 3);
        if (res != 3) {
            std::cerr << "Did not read 3 bytes\n";
            close(fd);
            return 5;
        }

        if (memcmp("TAG", tag_bytes, 3) == 0) {
            printf("ID3v1\n");
            handle_id3v1(fd);
        } else {

            // Should we add tags?
            char in = 0;
            while (in != 'n' && in != 'y') {
                printf("Add ID3v2 Tags? (y/n): ");
                std::cin >> in;
            }
            if (in == 'y') {
                uint8_t id3v2_header[10] = {'I','D','3',3,0,0, 0, 0, 0x1f, 0x76};
                add_bytes_at(fd, 10, id3v2_header, 0, argv[argc - 1]);
                handle_id3v2(fd, argv[argc - 1], 0);
                off_t curr = lseek(fd, 0, SEEK_CUR);

                // ID3v2 recommends adding padding to prevent having to rewrite large mp3s
                uint32_t bytes_to_write = 4096 - curr;

                uint8_t* zeroes = (uint8_t*) malloc(bytes_to_write);
                memset(zeroes, 0, bytes_to_write);

                // Add padding
                add_bytes(fd, bytes_to_write, zeroes, argv[argc - 1]);
            } else {
                in = 0;
                while (in != 'n' && in != 'y') {
                    printf("Add ID3v1 Tags? (y/n): ");
                    std::cin >> in;
                }
                if (in == 'y') {
                    char id3_frame[128] = {'T', 'A', 'G'};
                    add_bytes_at(fd, 128, (uint8_t*) id3_frame, end ,argv[argc - 1]);
                    lseek(fd, end + 3, SEEK_SET);
                    handle_id3v1(fd);
                }
            }
        }
    }

    // Close the file and return
    close(fd);
    return 0;
}