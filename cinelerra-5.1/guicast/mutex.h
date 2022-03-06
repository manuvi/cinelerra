
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef MUTEX_H
#define MUTEX_H

#include <stdio.h>
#include <mutex>
#include <thread>
#include <string>
#include <cstddef>
#include "bctrace.inc"

/**
 * @brief Native CINELERRA Mutex class
 * 
 */
class Mutex : public trace_info
{
public:
	/**
	 * @brief Construct a new Mutex object
	 * 
	 * @param title name used for debug purpose
	 * @param recursive define if the Mutex object can be locked from the owner
	 *   thread more times without going in deadlock state
	 */
	Mutex(const char *title = 0, int recursive = 0);
	
	/**
	 * @brief Destroy the Mutex object
	 * 
	 */
	~Mutex();

	/**
	 * @brief lock the Mutex on the caller thread id
	 * 
	 * @param location from where the Mutex lock is called, for debug purposes
	 * @return int always return 0
	 */
	int lock(const char *location = 0);
	
	/**
	 * @brief unlock the Mutex object. In recursion mode, if the object is locked more than
	 *   one time, the unlock has to be called 'counter' times to be really unlocked
	 * 
	 * @return int always return 0
	 */
	int unlock();

	/**
	 * @brief try to lock the Mutex object
	 * 
	 * @param location from where the Mutex lock is called, for debug purposes
	 * @return int could return EBUSY or zero or the state of try_lock 
	 */
	int trylock(const char *location = 0);
	
	/**
	 * @brief Internal structure reset
	 * 
	 * @return int always return 0
	 */
	int reset();

	/**
	 * @brief checks if the mutex has been already locked
	 * 
	 * @return bool true if locked, false otherwise
	 */
	bool is_locked();

	/**
	 * @brief Number of times the thread currently holding the mutex has locked it.
	 *   For recursive locking.
	 */
	int count;

	/**
	 * @brief ID of the thread currently holding the mutex.  For recursive locking.
	 * 
	 */
	std::thread::id thread_id;
		
	/**
	 * @brief recursive flag
	 * 
	 */
	int recursive;

	/**
	 * @brief Lock the variables for recursive locking.
	 * 
	 */
	std::mutex recursive_lock;

	/**
	 * @brief main mutex object
	 * 
	 */
	std::mutex mutex;

	/**
	 * @brief name of the Mutex object, for debug purposes
	 * 
	 */
	std::string title;
};

#endif
