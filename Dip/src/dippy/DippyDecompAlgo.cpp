#include "Python.h"

#include "DippyDecompAlgo.h"
#include "DippyDecompApp.h"
#include "DippyPythonUtils.h"

/**
 * Perform branching
 *
 * This function should populate the (down|up)Branch(LB|UB) vectors with (indices, bound-value) pairs
 * which define the branch.
 */
bool DippyAlgoMixin::chooseBranchSet(DecompAlgo* algo,
				     std::vector< std::pair<int, double> >& downBranchLB,
                                     std::vector< std::pair<int, double> >& downBranchUB,
                                     std::vector< std::pair<int, double> >& upBranchLB,
                                     std::vector< std::pair<int, double> >& upBranchUB)
{
   bool ret_val;

   if (!m_utilParam->GetSetting("pyBranchMethod", true)) {
      return algo->DecompAlgo::chooseBranchSet(downBranchLB, downBranchUB, 
					       upBranchLB, upBranchUB);
   }

   DippyDecompApp* app = (DippyDecompApp*)algo->getDecompApp();
   // copy the current solution into a Python list
   const double* xhat = algo->getXhat();
   PyObject* pSolutionList = pyTupleList_FromDoubleArray(xhat, app->m_colList);
   // try to call chooseBranchSet on the DipProblem python object
   PyObject* pResult = PyObject_CallMethod(m_pProb, "chooseBranchSet", "O", 
					   pSolutionList);

   if (pResult == NULL) {
      //something's gone wrong with the function call, a Python exception has 
      //been set 
      throw UtilException("Error calling method prob.chooseBranchSet()", 
			  "chooseBranchSet", "DippyDecompAlgo");
   }

   // need more error checking/assertion setting here
   if (pResult == Py_None) {
      // if chooseBranchSet returns None, do default branching for this algo
      ret_val = algo->DecompAlgo::chooseBranchSet(downBranchLB, downBranchUB, upBranchLB, upBranchUB);

      if (downBranchUB.size() > 0) {
         PyObject* downBranchVar, * upBranchVar;
         pDownLB = PyDict_New(); // Down branch LBs is an empty dictionary
         pDownUB = PyDict_New();
         downBranchVar = PyList_GetItem(app->m_colList, downBranchUB[0].first);
         PyDict_SetItem(pDownUB, downBranchVar, 
			PyInt_FromLong(static_cast<int>(round(downBranchUB[0].second))));
         pUpLB = PyDict_New();
         upBranchVar = PyList_GetItem(app->m_colList, upBranchLB[0].first);
         PyDict_SetItem(pUpLB, upBranchVar, 
			PyInt_FromLong(static_cast<int>(round(upBranchLB[0].second))));
         pUpUB = PyDict_New(); // Up branch UBs is an empty dictionary
         assert(downBranchVar == upBranchVar);
      }

      return ret_val;
   } else {
      // or else, the function should have returned 4 lists of (name, value) tuples
      pDownLB = PyTuple_GetItem(pResult, 0);
      pDownUB = PyTuple_GetItem(pResult, 1);
      pUpLB = PyTuple_GetItem(pResult, 2);
      pUpUB = PyTuple_GetItem(pResult, 3);
      // copy the python dictionaries into the result vectors
      pyColDict_AsPairedVector(pDownLB, downBranchLB, app->m_colIndices);
      pyColDict_AsPairedVector(pDownUB, downBranchUB, app->m_colIndices);
      pyColDict_AsPairedVector(pUpLB, upBranchLB, app->m_colIndices);
      pyColDict_AsPairedVector(pUpUB, upBranchUB, app->m_colIndices);
      return true;
   }
}

void DippyAlgoMixin::postProcessBranch(DecompAlgo* algo, 
				       DecompStatus decompStatus)
{
   PyObject* pOutput = PyList_New(0);

   if (!m_utilParam->GetSetting("pyPostProcessBranch", true)) {
      return;
   }

   AlpsDecompTreeNode* node = (AlpsDecompTreeNode*)algo->getCurrentNode();
   double quality = node->getQuality();

   if (pDownLB != NULL) {
      addTupleToPyList(pOutput, PyString_FromString("pDownLB"), pDownLB);
   }

   if (pDownUB != NULL) {
      addTupleToPyList(pOutput, PyString_FromString("pDownUB"), pDownUB);
   }

   if (pUpLB != NULL) {
      addTupleToPyList(pOutput, PyString_FromString("pUpLB"), pUpLB);
   }

   if (pUpUB != NULL) {
      addTupleToPyList(pOutput, PyString_FromString("pUpUB"), pUpUB);
   }

   addTupleToPyList(pOutput, PyString_FromString("nodeIndex"), PyInt_FromLong(algo->getNodeIndex()));
   addTupleToPyList(pOutput, PyString_FromString("nodeQuality"), PyFloat_FromDouble(quality));
   PyObject* pResult = PyObject_CallMethod(m_pProb, "postProcessBranch", "O", pOutput);
}

void DippyAlgoMixin::postProcessNode(DecompAlgo* algo, DecompStatus decompStatus)
{
   if (!m_utilParam->GetSetting("pyPostProcessNode", true)) {
      return;
   }

   PyObject* pOutput = pyTupleList_FromNode(algo, decompStatus);
   PyObject* pResult = PyObject_CallMethod(m_pProb, "postProcessNode", "O", pOutput);
}

