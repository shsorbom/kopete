/*
    Kopete Groupwise Protocol
    updatefoldertask.h - rename a folder on the server

    Copyright (c) 2004      SUSE Linux AG	 	 http://www.suse.com
    
    Based on Iris, Copyright (C) 2003  Justin Karneges

    Kopete (c) 2002-2004 by the Kopete developers <kopete-devel@kde.org>
 
    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef UPDATEFOLDERTASK_H
#define UPDATEFOLDERTASK_H

#include <updateitemtask.h>

/**
Renames a folder on the server

@author Kopete Developers
*/
class UpdateFolderTask : public UpdateItemTask
{
public:
    UpdateFolderTask(Task* parent);

    ~UpdateFolderTask();

};

#endif
