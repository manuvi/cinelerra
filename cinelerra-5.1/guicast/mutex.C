
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <iostream>

#include "bcsignals.h"
#include "mutex.h"


Mutex::Mutex(const char *title, int recursive) :	
	count(0),
	recursive(recursive)
{
	if ( title != nullptr ) 
	{
		this->title = std::string(title);
	}
}

Mutex::~Mutex()
{
	UNSET_ALL_LOCKS(this);
}

int Mutex::lock(const char *location)
{
	// Test recursive owner and give up if we already own it
	if( recursive )
	{
		recursive_lock.lock();
		if ( count > 0 )
		{
			if ( std::this_thread::get_id() == thread_id )
			{
				count ++;
				recursive_lock.unlock();
				return 0;
			}
		}
		recursive_lock.unlock();
	}

	SET_LOCK(this, title.c_str(), location);
	
	try {
		mutex.lock();
	} catch(const std::system_error& e) {
		std::cerr << "Mutex::lock with code" << e.code() << ":" << e.what() << std::endl;
	}
	set_owner();
	count = 1;
	if ( recursive )
	{
		thread_id = std::this_thread::get_id();
	}

	SET_LOCK2
	return 0;
}

int Mutex::unlock()
{
	if( count <= 0 ) {
		std::cerr << "Mutex::unlock not locked: " << title << std::endl;
		booby();
		return 0;
	}

	// Remove from recursive status
	if( recursive )
	{
		recursive_lock.lock();
		count --;
		
		// still locked
		if ( count > 0 )
		{
			recursive_lock.unlock();
			return 0;
		}
		recursive_lock.unlock();
	}
	else
	{
		count = 0;
	}

	UNSET_LOCK(this);
	unset_owner();

	try {
		mutex.unlock();
	} catch(const std::system_error& e) {
		std::cerr << "Mutex::unlock with code" << e.code() << ":" << e.what() << std::endl;
	}
	return 0;
}

int Mutex::trylock(const char *location)
{
	// Already locked
	if ( count ) 
	{
		return EBUSY;
	}

	if ( !mutex.try_lock() )
	{
		return 1;
	}
	set_owner();

	// Update recursive status for the first lock
	if( recursive ) 
	{
		recursive_lock.lock();
		count = 1;
		thread_id = std::this_thread::get_id();
		recursive_lock.unlock();
	}
	else
		count = 1;

	SET_LOCK(this, title.c_str(), location);
	SET_LOCK2
	return 0;
}

bool Mutex::is_locked()
{
	return count;
}

int Mutex::reset()
{
	UNSET_ALL_LOCKS(this)
	unset_owner();
	trace = 0;
	count = 0;
	return 0;
}
