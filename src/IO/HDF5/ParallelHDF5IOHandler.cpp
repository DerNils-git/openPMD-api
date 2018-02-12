/* Copyright 2017 Fabian Koller
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#include "IO/HDF5/ParallelHDF5IOHandler.hpp"


#if openPMD_HAVE_HDF5 && openPMD_HAVE_MPI
#include <iostream>

#include <mpi.h>
#include <boost/filesystem.hpp>

#include "auxiliary/StringManip.hpp"


#ifdef DEBUG
#define ASSERT(CONDITION, TEXT) { if(!(CONDITION)) throw std::runtime_error(std::string((TEXT))); }
#else
#define ASSERT(CONDITION, TEXT) do{ (void)sizeof(CONDITION); } while( 0 )
#endif


ParallelHDF5IOHandler::ParallelHDF5IOHandler(std::string const& path,
                                             AccessType at,
                                             MPI_Comm comm)
        : AbstractIOHandler(path, at, comm),
          m_impl{new ParallelHDF5IOHandlerImpl(this, comm)}
{ }

ParallelHDF5IOHandler::~ParallelHDF5IOHandler()
{ }

std::future< void >
ParallelHDF5IOHandler::flush()
{
    return m_impl->flush();
}

ParallelHDF5IOHandlerImpl::ParallelHDF5IOHandlerImpl(AbstractIOHandler* handler,
                                                     MPI_Comm comm)
        : HDF5IOHandlerImpl{handler},
          m_mpiComm{comm},
          m_mpiInfo{MPI_INFO_NULL} /* MPI 3.0+: MPI_INFO_ENV */
{
    m_datasetTransferProperty = H5Pcreate(H5P_DATASET_XFER);
    m_fileAccessProperty = H5Pcreate(H5P_FILE_ACCESS);
    herr_t status;
    status = H5Pset_dxpl_mpio(m_datasetTransferProperty, H5FD_MPIO_COLLECTIVE);
    ASSERT(status >= 0, "Internal error: Failed to set HDF5 dataset transfer property");
    status = H5Pset_fapl_mpio(m_fileAccessProperty, m_mpiComm, m_mpiInfo);
    ASSERT(status >= 0, "Internal error: Failed to set HDF5 file access property");
}

ParallelHDF5IOHandlerImpl::~ParallelHDF5IOHandlerImpl()
{
    herr_t status;
    while( !m_openFileIDs.empty() )
    {
        auto file = m_openFileIDs.begin();
        status = H5Fclose(*file);
        if( status < 0 )
            std::cerr << "Internal error: Failed to close HDF5 file (parallel)\n";
        m_openFileIDs.erase(file);
    }
}
#else
ParallelHDF5IOHandler::ParallelHDF5IOHandler(std::string const& path,
                                             AccessType at)
        : AbstractIOHandler(path, at)
{
    throw std::runtime_error("openPMD-api built without parallel HDF5 support");
}

ParallelHDF5IOHandler::~ParallelHDF5IOHandler()
{ }

std::future< void >
ParallelHDF5IOHandler::flush()
{
    return std::future< void >();
}
#endif
