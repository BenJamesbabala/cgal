#include "config.h"
#include "Scene_points_with_normal_item.h"
#include <CGAL/Three/Polyhedron_demo_plugin_helper.h>
#include <CGAL/Three/Polyhedron_demo_plugin_interface.h>

#include <CGAL/remove_outliers.h>
#include <CGAL/Timer.h>
#include <CGAL/Memory_sizer.h>

#include <QObject>
#include <QAction>
#include <QMainWindow>
#include <QApplication>
#include <QtPlugin>
#include <QInputDialog>
#include <QMessageBox>

#include "ui_Point_set_outliers_removal_plugin.h"
using namespace CGAL::Three;
class Polyhedron_demo_point_set_outliers_removal_plugin :
  public QObject,
  public Polyhedron_demo_plugin_helper
{
  Q_OBJECT
  Q_INTERFACES(CGAL::Three::Polyhedron_demo_plugin_interface)
  Q_PLUGIN_METADATA(IID "com.geometryfactory.PolyhedronDemo.PluginInterface/1.0")

private:
  QAction* actionOutlierRemoval;

public:
  void init(QMainWindow* mainWindow, CGAL::Three::Scene_interface* scene_interface, Messages_interface*) {
    scene = scene_interface;
    actionOutlierRemoval = new QAction(tr("Outliers Selection"), mainWindow);
    actionOutlierRemoval->setProperty("subMenuName","Point Set Processing");
    actionOutlierRemoval->setObjectName("actionOutlierRemoval");
    autoConnectActions();
  }
  
  //! Applicate for Point_sets with normals.
  bool applicable(QAction*) const {
    return qobject_cast<Scene_points_with_normal_item*>(scene->item(scene->mainSelectionIndex()));
  }

  QList<QAction*> actions() const {
    return QList<QAction*>() << actionOutlierRemoval;
  }

public Q_SLOTS:
  void on_actionOutlierRemoval_triggered();

}; // end Polyhedron_demo_point_set_outliers_removal_plugin

class Point_set_demo_outlier_removal_dialog : public QDialog, private Ui::OutlierRemovalDialog
{
  Q_OBJECT
  public:
    Point_set_demo_outlier_removal_dialog(QWidget * /*parent*/ = 0)
    {
      setupUi(this);
    }

    double percentage() const { return m_inputPercentage->value(); }
    int nbNeighbors() const { return m_inputNbNeighbors->value(); }
};

void Polyhedron_demo_point_set_outliers_removal_plugin::on_actionOutlierRemoval_triggered()
{
  const CGAL::Three::Scene_interface::Item_id index = scene->mainSelectionIndex();

  Scene_points_with_normal_item* item =
    qobject_cast<Scene_points_with_normal_item*>(scene->item(index));

  if(item)
  {
    // Gets point set
    Point_set* points = item->point_set();
    if(points == NULL)
        return;

    // Gets options
    Point_set_demo_outlier_removal_dialog dialog;
    if(!dialog.exec())
      return;
    const double removed_percentage = dialog.percentage(); // percentage of points to remove
    const int nb_neighbors = dialog.nbNeighbors();

    QApplication::setOverrideCursor(Qt::WaitCursor);

    CGAL::Timer task_timer; task_timer.start();
    std::cerr << "Select outliers (" << removed_percentage <<"%)...\n";

    // Computes outliers
    Point_set::iterator first_point_to_remove =
      CGAL::remove_outliers(points->begin(), points->end(),
                            points->point_map(),
                            nb_neighbors,
                            removed_percentage, Kernel());

    std::size_t nb_points_to_remove = std::distance(first_point_to_remove, points->end());
    std::size_t memory = CGAL::Memory_sizer().virtual_size();
    std::cerr << "Simplification: " << nb_points_to_remove << " point(s) are selected ("
                                    << task_timer.time() << " seconds, "
                                    << (memory>>20) << " Mb allocated)"
                                    << std::endl;

    // Selects points to delete
    points->set_first_selected (first_point_to_remove);

    // Updates scene
    item->invalidateOpenGLBuffers();
    scene->itemChanged(index);

    QApplication::restoreOverrideCursor();
  }
}

#include "Point_set_outliers_removal_plugin.moc"
