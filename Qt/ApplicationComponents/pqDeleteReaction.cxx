/*=========================================================================

   Program: ParaView
   Module:    pqDeleteReaction.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqDeleteReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkNew.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMViewProxy.h"

#include <QDebug>
#include <QSet>

//-----------------------------------------------------------------------------
pqDeleteReaction::pqDeleteReaction(QAction* parentObject, bool delete_all)
  : Superclass(parentObject)
{
  this->DeleteAll = delete_all;
  if (!this->DeleteAll)
    {
    QObject::connect(&pqActiveObjects::instance(),
      SIGNAL(portChanged(pqOutputPort*)),
      this, SLOT(updateEnableState()));

    //needed for when you delete the only consumer of an item. The selection
    //signal is emitted before the consumer is removed
    QObject::connect(pqApplicationCore::instance()->getServerManagerObserver(),
      SIGNAL(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*) ),
      this, SLOT(updateEnableState()) );
    }

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::updateEnableState()
{
  if (this->DeleteAll)
    {
    this->parentAction()->setEnabled(true);
    return;
    }

  this->parentAction()->setEnabled(this->canDeleteSelected());
}

//-----------------------------------------------------------------------------
static void pqDeleteReactionGetSelectedSet(
  vtkSMProxySelectionModel* selModel,
  QSet<pqPipelineSource*>& selectedSources)
{
  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  for (unsigned int cc=0; cc < selModel->GetNumberOfSelectedProxies(); cc++)
    {
    vtkSMProxy* proxy = selModel->GetSelectedProxy(cc);
    pqServerManagerModelItem* item =
      smmodel->findItem<pqServerManagerModelItem*>(proxy);
    pqOutputPort* port = qobject_cast<pqOutputPort*>(item);
    pqPipelineSource* source = port? port->getSource():
      qobject_cast<pqPipelineSource*>(item);
    if (source)
      {
      selectedSources.insert(source);
      }
    }
}

//-----------------------------------------------------------------------------
bool pqDeleteReaction::canDeleteSelected()
{
  vtkSMProxySelectionModel* selModel =
    pqActiveObjects::instance().activeSourcesSelectionModel();
  if (selModel == NULL || selModel->GetNumberOfSelectedProxies() == 0)
    {
    return false;
    }

  QSet<pqPipelineSource*> selectedSources;
  ::pqDeleteReactionGetSelectedSet(selModel, selectedSources);

  if (selectedSources.size() == 0)
    {
    return false;
    }

  // Now ensure that all consumers for the current sources don't have consumers
  // outside the selectedSources, then alone can we delete the selected items.
  foreach (pqPipelineSource* source, selectedSources)
    {
    QList<pqPipelineSource*> consumers = source->getAllConsumers();
    for (int cc=0; cc < consumers.size(); cc++)
      {
      pqPipelineSource* consumer = consumers[cc];
      if (consumer && !selectedSources.contains(consumer))
        {
        return false;
        }
      }
    }

  return true;
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::deleteAll()
{
  BEGIN_UNDO_EXCLUDE();
  if (pqServer* server = pqActiveObjects::instance().activeServer())
    {
    pqApplicationCore::instance()->getObjectBuilder()->resetServer(server);
    }
  END_UNDO_EXCLUDE();
  CLEAR_UNDO_STACK();
  pqApplicationCore::instance()->render();
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::deleteSelected()
{
  if (!pqDeleteReaction::canDeleteSelected())
    {
    qCritical() << "Cannot delete selected ";
    return;
    }

  vtkSMProxySelectionModel* selModel =
    pqActiveObjects::instance().activeSourcesSelectionModel();

  QSet<pqPipelineSource*> selectedSources;
  ::pqDeleteReactionGetSelectedSet(selModel, selectedSources);
  pqDeleteReaction::deleteSources(selectedSources);
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::deleteSource(pqPipelineSource* source)
{
  if (source)
    {
    QSet<pqPipelineSource*> sources;
    sources.insert(source);
    pqDeleteReaction::deleteSources(sources);
    }
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::deleteSources(QSet<pqPipelineSource*>& sources)
{
  if (sources.size() == 0) { return; }

  vtkNew<vtkSMParaViewPipelineController> controller;
  if (sources.size() == 1)
    {
    pqPipelineSource* source = (*sources.begin());
    BEGIN_UNDO_SET(QString("Delete %1").arg(source->getSMName()));
    }
  else
    {
    BEGIN_UNDO_SET("Delete Selection");
    }

  /// loop attempting to delete each source.
  bool something_deleted = false;
  bool something_deleted_in_current_iteration = false;
  do
    {
    something_deleted_in_current_iteration = false;
    foreach (pqPipelineSource* source, sources)
      {
      if (source && source->getNumberOfConsumers() == 0)
        {
        pqDeleteReaction::aboutToDelete(source);

        sources.remove(source);
        controller->UnRegisterProxy(source->getProxy());
        something_deleted = true;
        something_deleted_in_current_iteration = true;
        break;
        }
      }
    }
  while (something_deleted_in_current_iteration && (sources.size() > 0));

  // update scalar bars, if needed
  int sbMode = vtkPVGeneralSettings::GetInstance()->GetScalarBarMode();
  if (something_deleted &&
    (sbMode == vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS ||
     sbMode == vtkPVGeneralSettings::AUTOMATICALLY_HIDE_SCALAR_BARS))
    {
    vtkNew<vtkSMTransferFunctionManager> tmgr;
    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    QList<pqView*> views = smmodel->findItems<pqView*>();
    foreach (pqView* view, views)
      {
      tmgr->UpdateScalarBars(view->getProxy(),
        vtkSMTransferFunctionManager::HIDE_UNUSED_SCALAR_BARS);
      }
    }

  END_UNDO_SET();
  pqApplicationCore::instance()->render();
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::aboutToDelete(pqPipelineSource* source)
{
  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(source);
  if (!filter)
    {
    return;
    }

  pqOutputPort* firstInput = filter->getInput(filter->getInputPortName(0), 0);
  if (!firstInput)
    {
    return;
    }

  //---------------------------------------------------------------------------
  // Make input the active source is "source" was currently active.
  pqActiveObjects& activeObjects = pqActiveObjects::instance();
  if (activeObjects.activeSource() == source)
    {
    activeObjects.setActivePort(firstInput);
    }

  //---------------------------------------------------------------------------
  // Make input visible if it was hidden in views this source was displayed.
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  QList<pqView*> views = filter->getViews();
  foreach (pqView* view, views)
    {
    vtkSMViewProxy* viewProxy = view->getViewProxy();
    if (controller->GetVisibility(source->getSourceProxy(), 0, viewProxy))
      {
      // this will also hide scalar bars if needed.
      controller->Hide(source->getSourceProxy(), 0, viewProxy);

      // if firstInput had a representation in this view that was hidden, show
      // it. We don't want to create a new representation, however.
      if (viewProxy->FindRepresentation(
          firstInput->getSourceProxy(), firstInput->getPortNumber()))
        {
        vtkSMProxy* repr = controller->SetVisibility(
          firstInput->getSourceProxy(), firstInput->getPortNumber(),
          viewProxy, true);
        // since we turned on input representation, show scalar bar, if the user
        // preference is such.
        if (repr &&
          vtkPVGeneralSettings::GetInstance()->GetScalarBarMode() ==
          vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS &&
          vtkSMPVRepresentationProxy::GetUsingScalarColoring(repr))
          {
          vtkSMPVRepresentationProxy::SetScalarBarVisibility(repr, viewProxy, true);
          }
        view->render();
        }
      }
    }
  //---------------------------------------------------------------------------
}
