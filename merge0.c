/* merge0.c

   Bytewise merge two files' content if they differ only when one byte is NULL

   copyright (c) 2019-2022 Scott Jennings - All rights reserved as per GPLv3+:

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define VERSION "0.1.0"

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

// #include <linux/fcntl.h>
#define F_SETLEASE 1024

// #undef P_tmpdir
// #define P_tmpdir "/var/tmp"

#define READMODE "r+"
#define OPTIONLIST "hqbsefpv?"

void
usage( char * argv0 ) {
  fprintf( stderr, "\nusage: %s [-"OPTIONLIST"] <file1> <file2>\n", argv0);
};

void
help( char * argv0 ) {
  usage( argv0 );
  printf ("\t-h -? print usage help\n"
          "\t-q report only when files are changed\n"
	  "\t-b show only basenames in messages\n"
	  "\t-s require files to be of same size\n"
	  "\t-e allow appending to empty files.\n"
	  "\t-f force writing to files open by other programs\n"
	  "\t-p pretend, but change no files\n"
	  "\t-v show version number\n"
	  "\n"
	  "return codes:\n"
	  "\t01 - usage/help\n"
	  "\t02 - option error\n"
	  "\t03 - not exactly two files\n"
	  "\t04 - filesystem error\n"
	  "\t05 - not regular file\n"
	  "\t06 - files are hard linked\n"
	  "\t07 - files are different length\n"
	  "\t08 - empty file\n"
	  "\t09 - files have different non-zero data\n"
	  "\t66 - demonic influence\n"
	  "\n"
	  "If the content of the two files differ only where\n"
	  "one of the differing bytes at each offset is zero,\n"
	  "then they are bytewise merged by overwriting zeros in\n"
	  "one file with the non-zero data of the other file.\n"
	  "\n"
	  "Useful for merging two incomplete copies of the same\n"
	  "file, where the missing data is zero or \"sparse\".\n"
	  "\n"
	  "Exactly TWO files must be on the command line.\n"
	  "\n"
	  "By default, the smaller file is presumed to have\n"
	  "NULL data beyond it's end.\n"
	  "\n"
	  );
};

int
checkactive( FILE *fp ) {
  if ( fcntl(fileno(fp), F_SETLEASE, F_WRLCK) && EAGAIN == errno ) {
    /* file is opened by another program */
    return(1);
  };
  fcntl(fileno(fp), F_SETLEASE, F_UNLCK);
  return(0);
};

void
overwritefile( int writeactive, int common_count, int zero_count, char *percent_nonzero, FILE *inputFd, FILE *outputFd, char *outputfilepath, int pretend ) {
  int filechar;

  if ( 1 == pretend ) {
    printf( "pretending to update \"%s\" %s%% nonzero (%d of %d)\n", outputfilepath, percent_nonzero, common_count, common_count+zero_count );
    return;
  }
  
  if ( checkactive(outputFd) ) {
    if ( writeactive ) {
      printf( "updating =ACTIVE= \"%s\" %s%% nonzero (%d of %d)\n", outputfilepath, percent_nonzero, common_count, common_count+zero_count );
    } else {
      printf( "NOT updating =ACTIVE= \"%s\" %s%% nonzero (%d of %d)\n", outputfilepath, percent_nonzero, common_count, common_count+zero_count );
      return;
    }
  } else {
    printf( "updating \"%s\" %s%% nonzero (%d of %d)\n", outputfilepath, percent_nonzero, common_count, common_count+zero_count );
  };
  
  rewind( inputFd );
  rewind( outputFd );
  
  while ( ( filechar = fgetc( inputFd ) ) != EOF ) {
    if ( 0 != filechar ) {
      fputc( filechar, outputFd );
    } else {
      /* if writing NULLs, be sparse */
      fseek( outputFd, 1, SEEK_CUR );
    };
  };
  fflush( outputFd );
};

int
main(int argc, char *argv[]) {
  
  FILE *inputFd1, *inputFd2, *tempfileFd;
  int char1, char2, count;
  int changed_flag1=0, changed_flag2=0, common_count=0, zero_count=0, offset, tempfileFdi;
  int bequiet=0, besamesize=0, appendempty=0, writeactive=0, basenameonly=0, pretend=0;
  char *argv1, *argv2, *argv1out, *argv2out;
  char tmpfilepath[24]="/var/tmp/merge0-XXXXXX";
  char percent_nonzero[6];
  char opt;
  
  struct stat statbuf1, statbuf2, statbuft;

  while ( -1 != (opt = getopt (argc, argv, "?"OPTIONLIST)) )
    switch (opt) {
    case 'q':
      bequiet=1;
      break;
    case 'b':
      basenameonly=1;
      break;
    case 's':
      if ( 1 == appendempty ) {
	fprintf( stderr, "the \"e\" and \"s\" options are mutually exclusive\n" );
	return(2);
      }
      besamesize=1;
      break;
    case 'e':
      if ( 1 == besamesize ) {
	fprintf( stderr, "the \"e\" and \"s\" options are mutually exclusive\n" );
	return(2);
      }
      appendempty=1;
      break;
    case 'f':
      writeactive=1;
      break;
    case 'p':
      pretend=1;
      break;
    case 'h':
      help( argv[0] );
      return(1);
    case 'v':
      fprintf( stderr, "%s version %s\n\n", argv[0], VERSION );
      return(1);
    case '?':
      help( argv[0] );
      return(1);
    default:
      usage( argv[0] );
      return(2);
    }

  /* must be exactly two files on command line */

  while ( (optind < argc) && (0 == strnlen( argv[optind], 1 )) ) { optind++; }; /* ignore leading blank arguments */

  if ( 2 != ( argc - optind ) ) {
    usage( argv[0] );
    fprintf( stderr, "\nMust be exactly two files on command line. Found %i:\n", ( argc - optind ) );
    count = 1;
    while ( (argc - optind) >= count ) {
      fprintf( stderr, "%i: \"%s\"\n", count, argv[ optind + count - 1 ] );
      count++;
    }
    return(3);
  };

  argv1 = argv[optind];
  argv2 = argv[optind+1];
  
  /* how shall we display filenames? */
  
  if ( 1 == basenameonly ) {
    argv1out = basename( argv[optind] );
    argv2out = basename( argv[optind+1] );
  } else {
    argv1out = argv[optind];
    argv2out = argv[optind+1];
  };
  
  /* stat input files */
  
  if ( stat( argv1, &statbuf1) ) {
    fprintf( stderr, "\"%s\": %s\n", argv1, strerror( errno ) );
    return(4);
  };

  if ( stat( argv2, &statbuf2) ) {
    fprintf( stderr, "\"%s\": %s\n", argv2, strerror( errno ) );
    return(4);
  };

  if ( !S_ISREG(statbuf1.st_mode) ) {
    fprintf( stderr, "\"%s\" is not a regular file.\n", argv1out );
    return(5);
  };

  if ( !S_ISREG(statbuf2.st_mode) ) {
    fprintf( stderr, "\"%s\" is not a regular file.\n", argv2out );
    return(5);
  };

  if ( statbuf1.st_ino == statbuf2.st_ino ) {
    if ( !bequiet ) { fprintf( stderr, "files are the same inode\n" ); }
    return(6);
  }; 

  /* require files to be of same length? */

  if ( besamesize && ( statbuf1.st_size != statbuf2.st_size ) ) {
    if (!bequiet) { printf("files are of different lengths\n" ); };
    return(7);
  };
  
  /* Both empty? */
  
  if ( (0 == statbuf1.st_size) && (0 == statbuf2.st_size) ) {
    if (!bequiet) { printf("files are empty\n"); };
    return(8);
  };

  /* Allow appending to zero length files? */

  if ( !appendempty ) {
    if ( 0 == statbuf1.st_size ) {
      if (!bequiet) { printf("\"%s\" is empty\n", argv1out ); };
      return(8);
    };
    if ( 0 == statbuf2.st_size ) {
      if (!bequiet) { printf("\"%s\" is empty\n", argv2out ); };
      //      if (!bequiet) { printf("zero length: \"%s\"\n", argv2out ); };
      return(8);
    };
  };

  /* Open input files */
  
  inputFd1 = fopen( argv1, READMODE ); 
  if ( NULL == inputFd1 ) {
    fprintf( stderr, "error opening file %s\n", argv1 );
    return(4);
  };
  
  inputFd2 = fopen( argv2, READMODE );
  if ( NULL == inputFd2 ) {
    fprintf( stderr, "error opening file %s\n", argv2 );
    return(4);
  };

  /* open output temp file */
  
  umask(0);
  tempfileFdi = mkstemp(tmpfilepath);
  if ( -1 == tempfileFdi ) {
    perror("error creating temp file");
    return(4);
  };
  tempfileFd = fdopen( tempfileFdi, "w+b" );
  if ( NULL == tempfileFd ) {
    perror( "error opening temp file" );
    return(4);
  }
  if ( unlink(tmpfilepath) ) {
    perror( "error unlinking temp file" );
    return(4);
  };

  if ( fstat( tempfileFdi, &statbuft ) ) {
    perror( "failed to stat unlinked tempfile" );
    return(4);
  };

  /*  paranoia.... */
  if ( statbuft.st_nlink ) {
    fprintf( stderr, "tempfile has %lu nlinks! ino=%li size=%li\n", statbuft.st_nlink, statbuft.st_ino, statbuft.st_size );
    return(66);
  };
  if ( checkactive(tempfileFd) ) {
    fprintf( stderr, "tempfile is opened by another program!\n" );
    return(66);
  };

  if ( !bequiet ) {
    if ( checkactive(inputFd1) ) {
      fprintf( stderr, "\"%s\" is =ACTIVE=\n", argv1out );
    };
    if ( checkactive(inputFd2) ) {
      fprintf( stderr, "\"%s\" is =ACTIVE=\n", argv2out );
    };
  };
  
  /* Transfer data until we encounter end of input or an error */

  common_count = 0;
  zero_count = 0;
  changed_flag1 = 0;
  changed_flag2 = 0;
  offset = 0;
  while ( 1 ) {

    char1 = fgetc(inputFd1);
    char2 = fgetc(inputFd2);
    offset++;

    if ( char1 == char2 ) {
      if ( EOF == char1 ) { break; }; /* end of both files */
    } else {

      if ( EOF == char1 ) {
	char1 = char2;
	changed_flag1 = 1; /* argv2 overwrites argv1 */
      } else {
	if ( EOF == char2 ) {
	  changed_flag2 = 1; /* argv1 overwrites argv2 */
	} else {
	  if ( '\0' == char1 ) {
	    char1 = char2;
	    changed_flag1 = 1; /* argv2 overwrites argv1 */
	  } else {
	    if ( '\0' == char2 ) {
	      changed_flag2 = 1; /* argv1 overwrites argv2 */
	    } else {
	      if (!bequiet) {
		fprintf( stderr, "files have different non-zero data at offset %i\n", offset );
	      };
	      return(9);
	      
	    };
	  };
	};
      };
    };
    
    if ( 0 == char1 ) { zero_count++; } else { common_count++; };
    if( EOF == fputc(char1, tempfileFd) ) {
      perror( "tmpfile" );
      return(4);
    };

  };

  if ( 0 == (common_count+zero_count) ) {
    strcpy( percent_nonzero, "MT");
  } else {

    float f=((common_count*10000)/(zero_count+common_count));
    sprintf( percent_nonzero, "%.2f", f/100 );
    int i=0;
    while ( '.' != percent_nonzero[i] && '\0' != percent_nonzero[i] ) { i++; };
    if ( '.' == percent_nonzero[i] ) {
      i=strlen( percent_nonzero );
      while ( '0' == percent_nonzero[--i] ) { percent_nonzero[i] = '\0'; };
      if ( '.' == percent_nonzero[i] ) { percent_nonzero[i] = '\0'; };
    }

  }

  if ( (0 == changed_flag1) && (0 == changed_flag2) ) {
    if (!bequiet) { printf("files are identical, %s%% nonzero (%i of %i)\n", percent_nonzero, common_count, zero_count+common_count ); };
  } else {
    if ( changed_flag1 != 0 ) {
      overwritefile( writeactive, common_count, zero_count, percent_nonzero, tempfileFd, inputFd1, argv1, pretend );
    };
    if ( changed_flag2 != 0 ) {
      overwritefile( writeactive, common_count, zero_count, percent_nonzero, tempfileFd, inputFd2, argv2, pretend );
    };
  };

  if ( -1 == fclose(inputFd1) ) {
    perror( argv1 );
    return(4);
  };
  if ( -1 == fclose(inputFd2) ) {
    perror( argv2 );
    return(4);
  };
  if ( -1 == fclose(tempfileFd) ) {
    perror( argv2 );
    return(4);
  };
  
  return(0);
}






