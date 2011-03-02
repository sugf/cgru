#include "filequeue.h"

#include "afcommon.h"

#define AFOUTPUT
#undef AFOUTPUT
#include "../include/macrooutput.h"

FileQueue::FileQueue( const std::string & QueueName):
   AfQueue( QueueName)
{
}

FileQueue::~FileQueue()
{
}

void FileQueue::processItem( AfQueueItem* item) const
{
   FileData * filedata = (FileData*)item;
   AFINFA("FileQueue::processItem: \"%s\"\n", filedata->getFileName().toUtf8().data());
   int rotate = filedata->getRotate();
   std::string filename( filedata->getFileName());
   if( rotate > 0)
   {
#ifdef AFOUTPUT
printf("FileQueue::processItem: rotating \"%s\" %d times:\n", filename.c_str(), rotate);
#endif
      if( af::pathFileExists(filename))
      {
         renameNext( filename, 0, rotate);
         std::string newname = filename + ".0";
         rename( filename.c_str(), newname.c_str());
      }
   }
   else if( rotate == -1)
   {
      filename = filename + "." + af::time2str( time(NULL), "%y%m%d_%H%M%S");
   }
   AFCommon::writeFile( filedata->getData(), filedata->getLength(), filename);
}

void FileQueue::renameNext( const std::string & filename, int number, int maxnumber) const
{
   std::string curname = filename + "." + af::itos(number++);
   if( number >= maxnumber )
   {
#ifdef AFOUTPUT
printf("FileQueue::renameNext: removing \"%s\"\n", curname.toUtf8().data());
#endif
      if( remove( curname.c_str()) != 0 )
         AFERRPA("FileQueue::renameNext: unable to remove: \"%s\"\n", curname.c_str());
      return;
   }
   if( af::pathFileExists( curname) == false) return;

   std::string newname = filename + "." + af::itos(number);

   renameNext( filename, number, maxnumber);

#ifdef AFOUTPUT
printf("FileQueue::renameNext: renaming \"%s\" in \"%s\"\n", curname.toUtf8().data(), newname.toUtf8().data());
#endif
   if( rename( curname.c_str(), newname.c_str()) != 0)
      AFERRPA("FileQueue::renameNext: unable to rename: \"%s\" in \"%s\"\n",
         curname.c_str(), newname.c_str());
}
