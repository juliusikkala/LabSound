// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/HRTFDatabaseLoader.h"
#include "internal/Assertions.h"
#include "internal/HRTFDatabase.h"

#include <iostream>

namespace lab
{

std::shared_ptr<HRTFDatabaseLoader> HRTFDatabaseLoader::CreateHRTFLoader(
    float sampleRate,
    std::function<std::shared_ptr<AudioBus>(const std::string& path)>&& loaderCallback
){
    auto s_loader = std::make_shared<HRTFDatabaseLoader>(sampleRate, std::move(loaderCallback));
    s_loader->loadAsynchronously();
    return s_loader;
}

HRTFDatabaseLoader::HRTFDatabaseLoader(
    float sampleRate,
    std::function<std::shared_ptr<AudioBus>(const std::string& path)>&& loaderCallback
):    m_loading(false)
    , m_databaseSampleRate(sampleRate)
    , loaderCallback(loaderCallback)
{
}

HRTFDatabaseLoader::~HRTFDatabaseLoader()
{
    waitForLoaderThreadCompletion();

    if (m_databaseLoaderThread.joinable()) m_databaseLoaderThread.join();

    m_hrtfDatabase.reset();
}

// Asynchronously load the database in this thread.
void HRTFDatabaseLoader::databaseLoaderEntry(HRTFDatabaseLoader * threadData)
{
    std::lock_guard<std::mutex> locker(threadData->m_threadLock);
    HRTFDatabaseLoader * loader = reinterpret_cast<HRTFDatabaseLoader *>(threadData);
    ASSERT(loader);

    threadData->m_loading = true;
    loader->load();
    threadData->m_loadingCondition.notify_one();
}

void HRTFDatabaseLoader::load()
{
    m_hrtfDatabase.reset(new HRTFDatabase(m_databaseSampleRate, loaderCallback));

    if (!m_hrtfDatabase.get())
    {
        LOG_ERROR("HRTF database not loaded");
    }
}

void HRTFDatabaseLoader::loadAsynchronously()
{
    std::lock_guard<std::mutex> lock(m_threadLock);

    if (!m_hrtfDatabase.get() && !m_loading)
    {
        m_databaseLoaderThread = std::thread(databaseLoaderEntry, this);
    }
}

bool HRTFDatabaseLoader::isLoaded() const
{
    return (m_hrtfDatabase.get() != nullptr);
}

void HRTFDatabaseLoader::waitForLoaderThreadCompletion()
{
    std::unique_lock<std::mutex> locker(m_threadLock);
    while (!m_hrtfDatabase.get())
    {
        m_loadingCondition.wait(locker);
    }
}

}  // namespace lab
