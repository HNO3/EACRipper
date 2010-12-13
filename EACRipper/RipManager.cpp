#include "Defaults.h"

#include "RipManager.h"

#include "FileStream.h"

using namespace std;

namespace EACRipper
{
	RipManager::RipManager()
		: processorCount(1), runningThreads(0), mutex(nullptr), stop(false)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);

		processorCount = si.dwNumberOfProcessors;
	}

	RipManager::~RipManager()
	{
		stopRip();
	}

	bool RipManager::startRip(const TrackList &ilist, shared_ptr<RipCallbackDelegate> iprogressCallback)
	{
		tracks.clear();
		threads.clear();
		threadData.clear();

		threads.resize(processorCount);
		threadData.resize(processorCount);

		stop = false;

		list = shared_ptr<TrackList>(ilist.clone());
		size_t len = ilist.getTrackCount();

		progressCallback = shared_ptr<RipCallbackDelegate>(iprogressCallback);

		for(size_t i = 1; i <= len; ++ i)
			tracks.push_back(i);

		mutex = CreateMutexW(NULL, FALSE, NULL);
		
		// TODO: Read whole file?

		for(uint32_t i = 0; i < processorCount; ++ i)
		{
			InterlockedIncrement(&runningThreads);
			threadData[i].threadId = i;
			threads[i] = CreateThread(NULL, 0, ripThread, &threadData[i], 0, nullptr);
			if(threads[i] == nullptr)
				InterlockedDecrement(&runningThreads);
		}

		if(runningThreads == 0)
			return false;

		return true;
	}

	bool RipManager::stopRip()
	{
		stop = true;
		if(WaitForMultipleObjects(processorCount, &*threads.begin(), TRUE, INFINITE) == WAIT_FAILED)
			return false;
		if(mutex != nullptr)
		{
			CloseHandle(mutex);
			mutex = nullptr;
		}
		progressCallback.reset();
		return true;
	}

	bool RipManager::isRipping()
	{
		return runningThreads > 0;
	}

	namespace
	{
		class MutexSession
		{
		private:
			HANDLE mutex;
			bool wait;

		public:
			MutexSession(HANDLE, bool = false);
			~MutexSession();

			void waitSession();
			void releaseSession();
		};

		MutexSession::MutexSession(HANDLE imutex, bool iwait)
			: mutex(imutex), wait(false)
		{
			if(iwait)
				waitSession();
		}

		MutexSession::~MutexSession()
		{
			releaseSession();
		}

		void MutexSession::waitSession()
		{
			if(wait)
				return;

			if(WaitForSingleObject(mutex, INFINITE) != WAIT_FAILED)
				wait = true;
		}

		void MutexSession::releaseSession()
		{
			if(!wait)
				return;

			if(ReleaseMutex(mutex))
				wait = false;
		}
	}

	ulong32_t __stdcall RipManager::ripThread(void *param)
	{
		ThreadData *data = static_cast<ThreadData *>(param);
		RipManager &self = instance();
		size_t track;

		FileStreamReader fsr(static_cast<wstring>((*self.list)[L"SourcePath"]).c_str());

		while(!self.stop)
		{
			{
				MutexSession mutex(self.mutex, true);
				if(self.tracks.empty())
					break;
				track = self.tracks.front();
				self.tracks.pop_front();
			}

			// TODO: Open writing file stream
		}

		InterlockedDecrement(&self.runningThreads);
		
		return 0;
	}
}
