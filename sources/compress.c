#include "form3.h"

#ifdef WITHZLIB
/*
	Low level routines for dealing with zlib during sorting and handling
	the scratch files. Work started 5-sep-2005.
	The .sor file handling was more or less completed on 8-sep-2005
	The handling of the scratch files still needs some thinking.
	Complications are:
		gzip compression should be per expression, not per buffer.
		No gzip compression for expressions with a bracket index.
		Separate decompression buffers for expressions in the rhs.
		This last one will involve more buffer work and organization.
		Information about compression should be stored for each expr.
			(including what method/program was used)
	Note: Be careful with compression. By far the most compact method
	is the original problem....

  	#[ Variables :
*/

/*
	The following variables are to contain the intermediate buffers
	for the inflation of the various patches in the sort file.
	There can be up to MaxFpatches (FilePatches in the setup) and hence
	we can have that many streams simultaneously. We set this up once
	and only when needed.
*/
Bytef **ziobufnum;
Bytef *ziobuffers;

/*
  	#] Variables :
  	#[ SetupOutputGZIP :

	Routine prepares a gzip output stream for the given file.
*/

int
SetupOutputGZIP ARG1(FILEHANDLE *,f)
{
	GETIDENTITY;
	int zerror;

	if ( AT.SS != AM.S0 ) return(0);
	if ( AR.NoCompress == 1 ) return(0);
	if ( AR.gzipCompress <= 0 ) return(0);

	if ( f->ziobuffer == 0 ) {
/*
		1: Allocate a struct for the gzip stream:
*/
		f->zsp = Malloc1(sizeof(z_stream),"output zstream");
/*
		2: Allocate the output buffer.
*/
		f->ziobuffer =
			(Bytef *)Malloc1(f->ziosize*sizeof(char),"output zbuffer");
		if ( f->zsp == 0 || f->ziobuffer == 0 ) {
			LOCK(ErrorMessageLock);
			MesCall("SetupOutputGZIP");
			UNLOCK(ErrorMessageLock);
			Terminate(-1);
		}
	}
/*
	3: Set the default fields:
*/
	f->zsp->zalloc = Z_NULL;
	f->zsp->zfree  = Z_NULL;
    f->zsp->opaque = Z_NULL;
/*
	4: Set the output space:
*/
	f->zsp->next_out  = f->ziobuffer;
	f->zsp->avail_out = f->ziosize;
	f->zsp->total_out = 0;
/*
	5: Set the input space:
*/
	f->zsp->next_in  = (Bytef *)(f->PObuffer);
	f->zsp->avail_in = (Bytef *)(f->POfill) - (Bytef *)(f->PObuffer);
	f->zsp->total_in = 0;
/*
	6: Initiate the deflation
*/
	if ( ( zerror = deflateInit(f->zsp,AR.gzipCompress) ) != Z_OK ) {
		LOCK(ErrorMessageLock);
		MesPrint("Error from zlib: %s",f->zsp->msg);
		MesCall("SetupOutputGZIP");
		UNLOCK(ErrorMessageLock);
		Terminate(-1);
	}

	return(0);
}

/*
  	#] SetupOutputGZIP :
  	#[ PutOutputGZIP :

	Routine is called when the PObuffer of f is full.
	The contents of it will be compressed and whenever the output buffer
	f->ziobuffer is full it will be written and the output buffer
	will be reset.
	Upon exit the input buffer will be cleared.
*/

int
PutOutputGZIP ARG1(FILEHANDLE *,f)
{
	int zerror;
/*
	First set the number of bytes in the input
*/
	f->zsp->next_in  = (Bytef *)(f->PObuffer);
	f->zsp->avail_in = (Bytef *)(f->POfill) - (Bytef *)(f->PObuffer);
	f->zsp->total_in = 0;

	while ( ( zerror = deflate(f->zsp,Z_NO_FLUSH) ) == Z_OK ) {
		if ( f->zsp->avail_out == 0 ) {
/*
			Write the output
*/
			SeekFile(f->handle,&(f->POposition),SEEK_SET);
#ifdef GZIPDEBUG
			LOCK(ErrorMessageLock);
			MesPrint("Writing %l bytes at %10p",f->ziosize,&(f->POposition));
			UNLOCK(ErrorMessageLock);
#endif
			if ( WriteFile(f->handle,(UBYTE *)(f->ziobuffer),f->ziosize)
						!= f->ziosize ) {
				LOCK(ErrorMessageLock);
				MesPrint("Write error during compressed sort. Disk full?");
				UNLOCK(ErrorMessageLock);
				return(-1);
			}
			ADDPOS(f->filesize,f->ziosize);
			ADDPOS(f->POposition,f->ziosize);
/*
			Reset the output
*/
			f->zsp->next_out  = f->ziobuffer;
			f->zsp->avail_out = f->ziosize;
			f->zsp->total_out = 0;
		}
		else if ( f->zsp->avail_in == 0 ) {
/*
			Finish
*/
			return(0);
		}
		else {
			break;
		}
	}
	LOCK(ErrorMessageLock);
	MesPrint("Error in gzip handling of output.");
	UNLOCK(ErrorMessageLock);
	return(-1);
}

/*
  	#] PutOutputGZIP :
  	#[ FlushOutputGZIP :

	Routine is called to flush a stream. The compression of the input buffer
	will be completed and the contents of f->ziobuffer will be written.
	Both buffers will be cleared.
*/

int
FlushOutputGZIP ARG1(FILEHANDLE *,f)
{
	int zerror;
/*
	Set the proper parameters
*/
	f->zsp->next_in  = (Bytef *)(f->PObuffer);
	f->zsp->avail_in = (Bytef *)(f->POfill) - (Bytef *)(f->PObuffer);
	f->zsp->total_in = 0;

	while ( ( zerror = deflate(f->zsp,Z_FINISH) ) == Z_OK ) {
		if ( f->zsp->avail_out == 0 ) {
/*
			Write the output
*/
			SeekFile(f->handle,&(f->POposition),SEEK_SET);
#ifdef GZIPDEBUG
			LOCK(ErrorMessageLock);
			MesPrint("Writing %l bytes at %10p",f->ziosize,&(f->POposition));
			UNLOCK(ErrorMessageLock);
#endif
			if ( WriteFile(f->handle,(UBYTE *)(f->ziobuffer),f->ziosize)
						!= f->ziosize ) {
				LOCK(ErrorMessageLock);
				MesPrint("Write error during compressed sort. Disk full?");
				UNLOCK(ErrorMessageLock);
				return(-1);
			}
			ADDPOS(f->filesize,f->ziosize);
			ADDPOS(f->POposition,f->ziosize);
/*
			Reset the output
*/
			f->zsp->next_out  = f->ziobuffer;
			f->zsp->avail_out = f->ziosize;
			f->zsp->total_out = 0;
		}
	}
	if ( zerror == Z_STREAM_END ) {
/*
		Write the output
*/
		SeekFile(f->handle,&(f->POposition),SEEK_SET);
#ifdef GZIPDEBUG
		LOCK(ErrorMessageLock);
		MesPrint("Writing %l bytes at %10p",(LONG)(f->zsp->avail_out),&(f->POposition));
		UNLOCK(ErrorMessageLock);
#endif
		if ( WriteFile(f->handle,(UBYTE *)(f->ziobuffer),f->zsp->total_out)
						!= f->zsp->total_out ) {
			LOCK(ErrorMessageLock);
			MesPrint("Write error during compressed sort. Disk full?");
			UNLOCK(ErrorMessageLock);
			return(-1);
		}
		ADDPOS(f->filesize,f->zsp->total_out);
		ADDPOS(f->POposition,f->zsp->total_out);
#ifdef GZIPDEBUG
		LOCK(ErrorMessageLock);
		{  char *s = f->ziobuffer+f->zsp->total_out;
			MesPrint("   Last bytes written: %d %d %d %d %d",s[-5],s[-4],s[-3],s[-2],s[-1]);
		}
		MesPrint("     Perceived position in FlushOutputGZIP is %10p",&(f->POposition));
		UNLOCK(ErrorMessageLock);
#endif
/*
			Reset the output
*/
		f->zsp->next_out  = f->ziobuffer;
		f->zsp->avail_out = f->ziosize;
		f->zsp->total_out = 0;
        if ( ( zerror = deflateEnd(f->zsp) ) == Z_OK ) return(0);
		LOCK(ErrorMessageLock);
		if ( f->zsp->msg ) {
			MesPrint("Error in finishing gzip handling of output: %s",f->zsp->msg);
		}
		else {
			MesPrint("Error in finishing gzip handling of output.");
		}
		UNLOCK(ErrorMessageLock);
	}
	else {
		LOCK(ErrorMessageLock);
		MesPrint("Error in gzip handling of output.");
		UNLOCK(ErrorMessageLock);
	}
	return(-1);
}

/*
  	#] FlushOutputGZIP :
  	#[ SetupAllInputGZIP :

	Routine prepares all gzip input streams for a merge.
*/

int
SetupAllInputGZIP ARG1(SORTING *,S)
{
	int i, NumberOpened = 0;
	z_streamp zsp;

	if ( S->zsparray == 0 ) {
		S->zsparray = (z_streamp)Malloc1(sizeof(z_stream)*S->MaxFpatches,"input zstreams");
		if ( S->zsparray == 0 ) {
			LOCK(ErrorMessageLock);
			MesCall("SetupAllInputGZIP");
			UNLOCK(ErrorMessageLock);
			Terminate(-1);
		}
		ziobuffers = (Bytef *)Malloc1(S->MaxFpatches*S->file.ziosize*sizeof(Bytef),"input raw buffers");
		ziobufnum  = (Bytef **)Malloc1(S->MaxFpatches*S->file.ziosize*sizeof(Bytef *),"input raw pointers");
		if ( ziobuffers == 0 || ziobufnum == 0 ) {
			LOCK(ErrorMessageLock);
			MesCall("SetupAllInputGZIP");
			UNLOCK(ErrorMessageLock);
			Terminate(-1);
		}
		for ( i  = 0 ; i < S->MaxFpatches; i++ ) {
			ziobufnum[i] = ziobuffers + i * S->file.ziosize;
		}
	}
	for ( i = 0; i < S->inNum; i++ ) {
#ifdef GZIPDEBUG
		LOCK(ErrorMessageLock);
		MesPrint("Preparing z-stream %d with compression %d",i,S->fpincompressed[i]);
		UNLOCK(ErrorMessageLock);
#endif
		if ( S->fpincompressed[i] ) {
			zsp = &(S->zsparray[i]);
/*
			1: Set the default fields:
*/
			zsp->zalloc = Z_NULL;
			zsp->zfree  = Z_NULL;
			zsp->opaque = Z_NULL;
/*
			2: Set the output space:
*/
			zsp->next_out  = Z_NULL;
			zsp->avail_out = 0;
			zsp->total_out = 0;
/*
			3: Set the input space temporarily:
*/
			zsp->next_in  = Z_NULL;
			zsp->avail_in = 0;
			zsp->total_in = 0;
/*
			4: Initiate the inflation
*/
			if ( inflateInit(zsp) != Z_OK ) {
				LOCK(ErrorMessageLock);
				if ( zsp->msg ) MesPrint("Error from inflateInit: %s",zsp->msg);
				else            MesPrint("Error from inflateInit");
				MesCall("SetupAllInputGZIP");
				UNLOCK(ErrorMessageLock);
				Terminate(-1);
			}
			NumberOpened++;
		}
	}
	return(NumberOpened);
}

/*
  	#] SetupAllInputGZIP :
  	#[ FillInputGZIP :

	Routine is called when we need new input in the specified buffer.
	This buffer is used for the output and we keep reading and uncompressing
	input till either this buffer is full or the input stream is finished.
	The return value is the number of bytes in the buffer.
*/

LONG
FillInputGZIP ARG5(FILEHANDLE *,f,POSITION *,position,UBYTE *,buffer,LONG,buffersize,int,numstream)
{
	GETIDENTITY;
	int zerror;
	LONG readsize, toread;
	SORTING *S = AT.SS;
	z_streamp zsp;
	POSITION pos;
	if ( S->fpincompressed[numstream] ) {
		zsp = &(S->zsparray[numstream]);
		zsp->next_out = (Bytef *)buffer;
		zsp->avail_out = buffersize;
		zsp->total_out = 0;
		if ( zsp->avail_in == 0 ) {
/*
			First loading of the input
*/
			if ( ISGEPOSINC(S->fPatchesStop[numstream],*position,f->ziosize) ) {
				toread = f->ziosize;
			}
			else {
				DIFPOS(pos,S->fPatchesStop[numstream],*position);
				toread = (LONG)(BASEPOSITION(pos));
			}
			if ( toread > 0 ) {
#ifdef GZIPDEBUG
				LOCK(ErrorMessageLock);
				MesPrint("-+Reading %l bytes in stream %d at position %10p",toread,numstream,position);
				UNLOCK(ErrorMessageLock);
#endif
				SeekFile(f->handle,position,SEEK_SET);
				readsize = ReadFile(f->handle,(UBYTE *)(ziobufnum[numstream]),toread);
				SeekFile(f->handle,position,SEEK_CUR);
#ifdef GZIPDEBUG
				LOCK(ErrorMessageLock);
				{  char *s = ziobufnum[numstream]+readsize;
					MesPrint("  +Last bytes read: %d %d %d %d %d",s[-5],s[-4],s[-3],s[-2],s[-1]);
				}
				UNLOCK(ErrorMessageLock);
#endif
				if ( readsize == 0 ) {
					zsp->next_in  = ziobufnum[numstream];
					zsp->avail_in = f->ziosize;
					zsp->total_in = 0;
					return(zsp->total_out);
				}
				if ( readsize < 0 ) {
					LOCK(ErrorMessageLock);
					MesPrint("FillInputGZIP: Read error during compressed sort.");
					UNLOCK(ErrorMessageLock);
					return(-1);
				}
				ADDPOS(f->filesize,readsize);
				ADDPOS(f->POposition,readsize);
/*
				Set the input
*/
				zsp->next_in  = ziobufnum[numstream];
				zsp->avail_in = readsize;
				zsp->total_in = 0;
			}
		}
		while ( ( zerror = inflate(zsp,Z_NO_FLUSH) ) == Z_OK ) {
			if ( zsp->avail_out == 0 ) {
/*
				Finish
*/
				return((LONG)(zsp->total_out));
			}
			if ( zsp->avail_in == 0 ) {
				
				if ( ISEQUALPOS(S->fPatchesStop[numstream],*position) ) {
/*
					We finished this stream. Try to terminate.
*/
					if ( ( zerror = inflate(zsp,Z_SYNC_FLUSH) ) == Z_OK ) {
						return((LONG)(zsp->total_out));
					}
					else
						break;
#ifdef GZIPDEBUG
					LOCK(ErrorMessageLock);
					MesPrint("Closing stream %d",numstream);
#endif
					readsize = zsp->total_out;
#ifdef GZIPDEBUG
					if ( readsize > 0 ) {
						WORD *s = (WORD *)(buffer+zsp->total_out);
						MesPrint("   Last words: %d %d %d %d %d",s[-5],s[-4],s[-3],s[-2],s[-1]);
					}
					else {
						MesPrint(" No words");
					}
					UNLOCK(ErrorMessageLock);
#endif
					if ( inflateEnd(zsp) == Z_OK ) return(readsize);
					break;
				}
/*
				Read more input
*/
#ifdef GZIPDEBUG
				if ( numstream == 0 ) {
					LOCK(ErrorMessageLock);
					MesPrint("Want to read in stream 0 at position %10p",position);
					UNLOCK(ErrorMessageLock);
				}
#endif
				if ( ISGEPOSINC(S->fPatchesStop[numstream],*position,f->ziosize) ) {
					toread = f->ziosize;
				}
				else {
					DIFPOS(pos,S->fPatchesStop[numstream],*position);
					toread = (LONG)(BASEPOSITION(pos));
				}
#ifdef GZIPDEBUG
				LOCK(ErrorMessageLock);
				MesPrint("--Reading %l bytes in stream %d at position %10p",toread,numstream,position);
				UNLOCK(ErrorMessageLock);
#endif
				SeekFile(f->handle,position,SEEK_SET);
				readsize = ReadFile(f->handle,(UBYTE *)(ziobufnum[numstream]),toread);
				SeekFile(f->handle,position,SEEK_CUR);
#ifdef GZIPDEBUG
				LOCK(ErrorMessageLock);
				{  char *s = ziobufnum[numstream]+readsize;
					MesPrint("   Last bytes read: %d %d %d %d %d",s[-5],s[-4],s[-3],s[-2],s[-1]);
				}
				UNLOCK(ErrorMessageLock);
#endif
				if ( readsize == 0 ) {
					zsp->next_in  = ziobufnum[numstream];
					zsp->avail_in = f->ziosize;
					zsp->total_in = 0;
					return(zsp->total_out);
				}
				if ( readsize < 0 ) {
					LOCK(ErrorMessageLock);
					MesPrint("FillInputGZIP: Read error during compressed sort.");
					UNLOCK(ErrorMessageLock);
					return(-1);
				}
				ADDPOS(f->filesize,readsize);
				ADDPOS(f->POposition,readsize);
/*
				Reset the input
*/
				zsp->next_in  = ziobufnum[numstream];
				zsp->avail_in = readsize;
				zsp->total_in = 0;
			}
			else {
				break;
			}
		}
#ifdef GZIPDEBUG
			LOCK(ErrorMessageLock);
			MesPrint(" zerror = %d in stream %d. At position %10p",zerror,numstream,position);
			UNLOCK(ErrorMessageLock);
#endif
		if ( zerror == Z_STREAM_END ) {
/*
			Reset the input
*/
			zsp->next_in  = Z_NULL;
			zsp->avail_in = 0;
			zsp->total_in = 0;
/*
			Make the final call and finish
*/
#ifdef GZIPDEBUG
			LOCK(ErrorMessageLock);
			MesPrint("Closing stream %d",numstream);
#endif
			readsize = zsp->total_out;
#ifdef GZIPDEBUG
			if ( readsize > 0 ) {
				WORD *s = (WORD *)(buffer+zsp->total_out);
				MesPrint("  -Last words: %d %d %d %d %d",s[-5],s[-4],s[-3],s[-2],s[-1]);
			}
			else {
				MesPrint(" No words");
			}
			UNLOCK(ErrorMessageLock);
#endif
			if ( inflateEnd(zsp) == Z_OK ) return(readsize);
		}

		LOCK(ErrorMessageLock);
		MesPrint("FillInputGZIP: Error in gzip handling of input.");
		UNLOCK(ErrorMessageLock);
		return(-1);
	}
	else {
#ifdef GZIPDEBUG
		LOCK(ErrorMessageLock);
		MesPrint("++Reading %l bytes at position %10p",buffersize,position);
		UNLOCK(ErrorMessageLock);
#endif
		SeekFile(f->handle,position,SEEK_SET);
		if ( ( readsize = ReadFile(f->handle,buffer,buffersize) ) < 0 ) {
			LOCK(ErrorMessageLock);
			MesPrint("FillInputGZIP: Read error during uncompressed sort.");
			UNLOCK(ErrorMessageLock);
			return(-1);
		}
		SeekFile(f->handle,position,SEEK_CUR);
		return(readsize);
	}
}

/*
  	#] FillInputGZIP :
*/
#endif