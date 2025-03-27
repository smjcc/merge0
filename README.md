# merge0: Bytewise merge two incomplete files

merge0 [-hqbsefpv?] \<file1\> \<file2\>

merge0 bytewise merges two files' content if they differ only where one byte is 0 (NULL)

If the content of the two files differ only where one of the differing bytes at each offset is zero, then they are bytewise merged by overwriting zeros in one file with the non-zero data of the other file.

## OPTIONS:
	-h -? print usage help
	-q report only when files are changed
	-b show only basenames in messages
	-s require files to be of same size
	-e allow appending to empty files.
	-f force writing to files open by other programs
	-p pretend, but change no files
	-v show version number

Useful for merging two incomplete copies of the same file, where the missing data is zero or "sparse".

Exactly TWO files must be on the command line.

By default, the smaller file is presumed to have NULL data beyond it's end.
