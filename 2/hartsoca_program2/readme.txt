How to compile:

$ gcc --std=gnu99 -o movies_by_year movies.c
$ /path/to/executable/movies

Example:

$ gcc --std=gnu99 -o movies_by_year movies.c
$ /path/to/executable/movies

If it doesn't compile or if you have any questions then email me at hartsoca@oregonstate.edu. 
I have successfully compiled and ran on the os1 server and it worked on April 4 4pm. 
This program will fail if you select options 1 or 2 (largest and smallest file) and there are no files with the prefix movies_ or .csv extension in the working directory. 
This was not a functionality mentioned so I didn't implement it. 
