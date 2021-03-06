/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2017 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::OFstreamCollator

Description
    Threaded file writer.

    Collects all data from all processors and writes as single
    'decomposedBlockData' file. The operation is determined by the
    buffer size (maxThreadFileBufferSize setting):
    - local size of data is larger than buffer: receive and write processor
    by processor (i.e. 'scheduled'). Does not use a thread, no file size
    limit.
    - total size of data is larger than buffer (but local is not):
    thread does all the collecting and writing of the processors. No file
    size limit.
    - total size of data is less than buffer:
    collecting is done locally; the thread only does the writing
    (since the data has already been collected)


Operation determine

SourceFiles
    OFstreamCollator.C

\*---------------------------------------------------------------------------*/

#ifndef OFstreamCollator_H
#define OFstreamCollator_H

#include "IOstream.H"
#include "labelList.H"
#include "FIFOStack.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                        Class OFstreamCollator Declaration
\*---------------------------------------------------------------------------*/

class OFstreamCollator
{
    // Private class

        class writeData
        {
        public:

            const label comm_;
            const word typeName_;
            const fileName pathName_;
            const string data_;
            const labelList sizes_;

            const bool haveSlaveData_;
            List<char> slaveData_;

            const IOstream::streamFormat format_;
            const IOstream::versionNumber version_;
            const IOstream::compressionType compression_;
            const bool append_;

            writeData
            (
                const label comm,
                const word& typeName,
                const fileName& pathName,
                const string& data,
                const labelList& sizes,
                const bool haveSlaveData,
                IOstream::streamFormat format,
                IOstream::versionNumber version,
                IOstream::compressionType compression,
                const bool append
            )
            :
                comm_(comm),
                typeName_(typeName),
                pathName_(pathName),
                data_(data),
                sizes_(sizes),
                haveSlaveData_(haveSlaveData),
                slaveData_(0),
                format_(format),
                version_(version),
                compression_(compression),
                append_(append)
            {}

            //- (approximate) size of master + any optional slave data
            off_t size() const
            {
                return data_.size() + slaveData_.size();
            }
        };


    // Private data

        const off_t maxBufferSize_;

        //pthread_mutex_t mutex_;
        label mutex_;

        //pthread_t thread_;
        label thread_;

        FIFOStack<writeData*> objects_;

        bool threadRunning_;

        //- Communicator to use for all parallel ops
        label comm_;


    // Private Member Functions

        //- Write actual file
        static bool writeFile
        (
            const label comm,
            const word& typeName,
            const fileName& fName,
            const string& masterData,
            const labelUList& recvSizes,
            const bool haveSlaveData,       // (does master) have slave data
            const UList<char>& slaveData,   // (on master) all slave data
            IOstream::streamFormat fmt,
            IOstream::versionNumber ver,
            IOstream::compressionType cmp,
            const bool append
        );

        //- Write all files in stack
        static void* writeAll(void *threadarg);

        //- Wait for total size of objects_ (master + optional slave data)
        //  to be wantedSize less than overall maxBufferSize.
        void waitForBufferSpace(const off_t wantedSize) const;


public:

    // Declare name of the class and its debug switch
    TypeName("OFstreamCollator");


    // Constructors

        //- Construct from buffer size. 0 = do not use thread
        OFstreamCollator(const off_t maxBufferSize);


    //- Destructor
    virtual ~OFstreamCollator();


    // Member functions

        //- Write file with contents. Blocks until writethread has space
        //  available (total file sizes < maxBufferSize)
        bool write
        (
            const word& typeName,
            const fileName&,
            const string& data,
            IOstream::streamFormat,
            IOstream::versionNumber,
            IOstream::compressionType,
            const bool append
        );
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
