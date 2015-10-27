#ifndef SCENE_C3T3_ITEM_H
#define SCENE_C3T3_ITEM_H

#include "Scene_c3t3_item_config.h"
#include "C3t3_type.h"

#include <QVector>
#include <QColor>
#include <QPixmap>
#include <QMenu>
#include <set>

#include <QtCore/qglobal.h>
#include <CGAL/gl.h>
#include <QGLViewer/manipulatedFrame.h>
#include <QGLViewer/qglviewer.h>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

#include <CGAL/Three/Viewer_interface.h>

#include <Scene_item.h>
#include <Scene_polyhedron_item.h>
#include <Scene_polygon_soup_item.h>

struct Scene_c3t3_item_priv;

using namespace CGAL::Three;
  class SCENE_C3T3_ITEM_EXPORT Scene_c3t3_item
  : public Scene_item
{
  Q_OBJECT
public:
  typedef qglviewer::ManipulatedFrame ManipulatedFrame;

  Scene_c3t3_item();
  Scene_c3t3_item(const C3t3& c3t3);
  ~Scene_c3t3_item();

  void invalidate_buffers()
  {
    are_buffers_filled = false;
  }

  void c3t3_changed();

  void contextual_changed()
  {
    if (frame->isManipulated() || frame->isSpinning())
      invalidate_buffers();
  }
  const C3t3& c3t3() const;
  C3t3& c3t3();

  bool manipulatable() const {
    return true;
  }
  ManipulatedFrame* manipulatedFrame() {
    return frame;
  }

  void setPosition(float x, float y, float z) {
    frame->setPosition(x, y, z);
  }

  void setNormal(float x, float y, float z) {
    frame->setOrientation(x, y, z, 0.f);
  }

  Kernel::Plane_3 plane() const {
    const qglviewer::Vec& pos = frame->position();
    const qglviewer::Vec& n =
      frame->inverseTransformOf(qglviewer::Vec(0.f, 0.f, 1.f));
    return Kernel::Plane_3(n[0], n[1], n[2], -n * pos);
  }

  bool isFinite() const { return true; }
  bool isEmpty() const {
    return c3t3().triangulation().number_of_vertices() == 0;
  }

  Bbox bbox() const {
    if (isEmpty())
      return Bbox();
    else {
      CGAL::Bbox_3 result = c3t3().triangulation().finite_vertices_begin()->point().bbox();
      for (Tr::Finite_vertices_iterator
        vit = ++c3t3().triangulation().finite_vertices_begin(),
        end = c3t3().triangulation().finite_vertices_end();
      vit != end; ++vit)
      {
        result = result + vit->point().bbox();
      }
      return Bbox(result.xmin(), result.ymin(), result.zmin(),
        result.xmax(), result.ymax(), result.zmax());
    }
  }

  Scene_c3t3_item* clone() const {
    return 0;
  }

  // data item
  inline const Scene_item* data_item() const;
  inline void set_data_item(const Scene_item* data_item);



  QString toolTip() const {
    int number_of_tets = 0;
    for (Tr::Finite_cells_iterator
      cit = c3t3().triangulation().finite_cells_begin(),
      end = c3t3().triangulation().finite_cells_end();
    cit != end; ++cit)
    {
      if (c3t3().is_in_complex(cit))
        ++number_of_tets;
    }
    return tr("<p><b>3D complex in a 3D triangulation</b></p>"
      "<p>Number of vertices: %1<br />"
      "Number of surface facets: %2<br />"
      "Number of volume tetrahedra: %3</p>")
      .arg(c3t3().triangulation().number_of_vertices())
      .arg(c3t3().number_of_facets())
      .arg(number_of_tets);
  }

  // Indicate if rendering mode is supported
  bool supportsRenderingMode(RenderingMode m) const {
    return (m != Gouraud && m != PointsPlusNormals && m != Splatting); // CHECK THIS!
  }

  void draw(CGAL::Three::Viewer_interface* viewer) const {
    if (!are_buffers_filled)
    {
      compute_elements();
      initialize_buffers(viewer);
    }
    vaos[0]->bind();
    program = getShaderProgram(PROGRAM_WITH_LIGHT);
    attrib_buffers(viewer, PROGRAM_WITH_LIGHT);
    program->bind();
    program->setAttributeValue("colors", this->color());
    viewer->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(positions_poly.size() / 3));
    program->release();
    vaos[0]->release();


  }
  void draw_edges(CGAL::Three::Viewer_interface* viewer) const {
    if (!are_buffers_filled)
    {
      compute_elements();
      initialize_buffers(viewer);
    }
    vaos[2]->bind();
    program = getShaderProgram(PROGRAM_WITHOUT_LIGHT);
    attrib_buffers(viewer, PROGRAM_WITHOUT_LIGHT);
    program->bind();
    program->setAttributeValue("colors", QColor(Qt::black));
    QMatrix4x4 f_mat;
    for (int i = 0; i<16; i++)
      f_mat.data()[i] = frame->matrix()[i];
    program->setUniformValue("f_matrix", f_mat);
    viewer->glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(positions_grid.size() / 3));
    program->release();
    vaos[2]->release();

    vaos[1]->bind();
    program = getShaderProgram(PROGRAM_WITHOUT_LIGHT);
    attrib_buffers(viewer, PROGRAM_WITHOUT_LIGHT);
    program->bind();
    program->setAttributeValue("colors", QColor(Qt::black));
    viewer->glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(positions_lines.size() / 3));
    program->release();
    vaos[1]->release();

  }
  void draw_points(CGAL::Three::Viewer_interface * viewer) const
  {
    if (!are_buffers_filled)
    {
      compute_elements();
      initialize_buffers(viewer);
    }
    vaos[1]->bind();
    program = getShaderProgram(PROGRAM_WITHOUT_LIGHT);
    attrib_buffers(viewer, PROGRAM_WITHOUT_LIGHT);
    program->bind();
    program->setAttributeValue("colors", this->color());
    viewer->glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(positions_lines.size() / 3));
    vaos[1]->release();
    program->release();

    vaos[2]->bind();
    program = getShaderProgram(PROGRAM_WITHOUT_LIGHT);
    attrib_buffers(viewer, PROGRAM_WITHOUT_LIGHT);
    program->bind();
    program->setAttributeValue("colors", this->color());
    QMatrix4x4 f_mat;
    for (int i = 0; i<16; i++)
      f_mat.data()[i] = frame->matrix()[i];
    program->setUniformValue("f_matrix", f_mat);
    viewer->glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(positions_grid.size() / 3));
    program->release();
    vaos[2]->release();
  }
private:
  void draw_triangle(const Kernel::Point_3& pa,
    const Kernel::Point_3& pb,
    const Kernel::Point_3& pc, bool /* is_cut */) const {

#undef darker
    Kernel::Vector_3 n = cross_product(pb - pa, pc - pa);
    n = n / CGAL::sqrt(n*n);


    for (int i = 0; i<3; i++)
    {
      normals.push_back(n.x());
      normals.push_back(n.y());
      normals.push_back(n.z());
    }
    positions_poly.push_back(pa.x());
    positions_poly.push_back(pa.y());
    positions_poly.push_back(pa.z());

    positions_poly.push_back(pb.x());
    positions_poly.push_back(pb.y());
    positions_poly.push_back(pb.z());

    positions_poly.push_back(pc.x());
    positions_poly.push_back(pc.y());
    positions_poly.push_back(pc.z());

  }

  void draw_triangle_edges(const Kernel::Point_3& pa,
    const Kernel::Point_3& pb,
    const Kernel::Point_3& pc)const {

#undef darker
    Kernel::Vector_3 n = cross_product(pb - pa, pc - pa);
    n = n / CGAL::sqrt(n*n);
    positions_lines.push_back(pa.x());
    positions_lines.push_back(pa.y());
    positions_lines.push_back(pa.z());

    positions_lines.push_back(pb.x());
    positions_lines.push_back(pb.y());
    positions_lines.push_back(pb.z());

    positions_lines.push_back(pb.x());
    positions_lines.push_back(pb.y());
    positions_lines.push_back(pb.z());

    positions_lines.push_back(pc.x());
    positions_lines.push_back(pc.y());
    positions_lines.push_back(pc.z());

    positions_lines.push_back(pc.x());
    positions_lines.push_back(pc.y());
    positions_lines.push_back(pc.z());

    positions_lines.push_back(pa.x());
    positions_lines.push_back(pa.y());
    positions_lines.push_back(pa.z());

  }



  double complex_diag() const {
    const Bbox& bbox = this->bbox();
    const double& xdelta = bbox.xmax - bbox.xmin;
    const double& ydelta = bbox.ymax - bbox.ymin;
    const double& zdelta = bbox.zmax - bbox.zmin;
    const double diag = std::sqrt(xdelta*xdelta +
      ydelta*ydelta +
      zdelta*zdelta);
    return diag * 0.7;
  }

  void compute_color_map(const QColor& c);

  public Q_SLOTS:
  void export_facets_in_complex()
  {
    std::stringstream off_sstream;
    c3t3().output_facets_in_complex_to_off(off_sstream);
    std::string backup = off_sstream.str();
    // Try to read .off in a polyhedron
    Scene_polyhedron_item* item = new Scene_polyhedron_item;
    if (!item->load(off_sstream))
    {
      delete item;
      off_sstream.str(backup);

      // Try to read .off in a polygon soup
      Scene_polygon_soup_item* soup_item = new Scene_polygon_soup_item;

      if (!soup_item->load(off_sstream)) {
        delete soup_item;
        return;
      }

      soup_item->setName(QString("%1_%2").arg(this->name()).arg("facets"));
      last_known_scene->addItem(soup_item);
    }
    else{
      item->setName(QString("%1_%2").arg(this->name()).arg("facets"));
      last_known_scene->addItem(item);
    }
  }

  inline void data_item_destroyed();

  //ici
  virtual QPixmap graphicalToolTip() const;
  inline void update_histogram();

private:
  void build_histogram();
  QColor get_histogram_color(const double v) const;

public:

  QMenu* contextMenu()
  {
    const char* prop_name = "Menu modified by Scene_c3t3_item.";

    QMenu* menu = Scene_item::contextMenu();

    // Use dynamic properties:
    // http://doc.qt.io/qt-5/qobject.html#property
    bool menuChanged = menu->property(prop_name).toBool();

    if (!menuChanged) {
      QAction* actionExportFacetsInComplex =
        menu->addAction(tr("Export facets in complex"));
      actionExportFacetsInComplex->setObjectName("actionExportFacetsInComplex");
      connect(actionExportFacetsInComplex,
        SIGNAL(triggered()), this,
        SLOT(export_facets_in_complex()));
    }
    return menu;
  }

  void set_scene(CGAL::Three::Scene_interface* scene){ last_known_scene = scene; }

protected:
  Scene_c3t3_item_priv* d;

private:
  qglviewer::ManipulatedFrame* frame;
  CGAL::Three::Scene_interface* last_known_scene;

  const Scene_item* data_item_;
  QPixmap histogram_;

  typedef std::set<int> Indices;
  Indices indices_;

  mutable std::vector<float> positions_lines;
  mutable std::vector<float> positions_grid;
  mutable std::vector<float> positions_poly;
  mutable std::vector<float> normals;


  mutable QOpenGLShaderProgram *program;

  using Scene_item::initialize_buffers;
  void initialize_buffers(CGAL::Three::Viewer_interface *viewer)const
  {
    //vao containing the data for the facets
    {
      program = getShaderProgram(PROGRAM_WITH_LIGHT, viewer);
      program->bind();

      vaos[0]->bind();
      buffers[0].bind();
      buffers[0].allocate(positions_poly.data(),
        static_cast<int>(positions_poly.size()*sizeof(float)));
      program->enableAttributeArray("vertex");
      program->setAttributeBuffer("vertex", GL_FLOAT, 0, 3);
      buffers[0].release();

      buffers[1].bind();
      buffers[1].allocate(normals.data(),
        static_cast<int>(normals.size()*sizeof(float)));
      program->enableAttributeArray("normals");
      program->setAttributeBuffer("normals", GL_FLOAT, 0, 3);
      buffers[1].release();

      vaos[0]->release();
      program->release();

    }

    //vao containing the data for the lines
        {
          program = getShaderProgram(PROGRAM_WITHOUT_LIGHT, viewer);
          program->bind();

          vaos[1]->bind();
          buffers[2].bind();
          buffers[2].allocate(positions_lines.data(),
            static_cast<int>(positions_lines.size()*sizeof(float)));
          program->enableAttributeArray("vertex");
          program->setAttributeBuffer("vertex", GL_FLOAT, 0, 3);
          buffers[2].release();

          vaos[1]->release();
          program->release();

        }

        //vao containing the data for the grid
        {
          program = getShaderProgram(PROGRAM_WITHOUT_LIGHT, viewer);
          program->bind();

          vaos[2]->bind();
          buffers[3].bind();
          buffers[3].allocate(positions_grid.data(),
            static_cast<int>(positions_grid.size()*sizeof(float)));
          program->enableAttributeArray("vertex");
          program->setAttributeBuffer("vertex", GL_FLOAT, 0, 3);
          buffers[3].release();
          vaos[2]->release();
          program->release();
        }
        are_buffers_filled = true;
  }
  void compute_elements() const
  {
    positions_lines.clear();
    positions_poly.clear();
    normals.clear();

    //The grid
    {
      float x = (2 * (float)complex_diag()) / 10.0;
      float y = (2 * (float)complex_diag()) / 10.0;
      for (int u = 0; u < 11; u++)
      {

        positions_grid.push_back(-(float)complex_diag() + x* u);
        positions_grid.push_back(-(float)complex_diag());
        positions_grid.push_back(0.0);

        positions_grid.push_back(-(float)complex_diag() + x* u);
        positions_grid.push_back((float)complex_diag());
        positions_grid.push_back(0.0);
      }
      for (int v = 0; v<11; v++)
      {

        positions_grid.push_back(-(float)complex_diag());
        positions_grid.push_back(-(float)complex_diag() + v * y);
        positions_grid.push_back(0.0);

        positions_grid.push_back((float)complex_diag());
        positions_grid.push_back(-(float)complex_diag() + v * y);
        positions_grid.push_back(0.0);
      }
    }


    if (isEmpty())
      return;

    const Kernel::Plane_3& plane = this->plane();
    GLdouble clip_plane[4];
    clip_plane[0] = -plane.a();
    clip_plane[1] = -plane.b();
    clip_plane[2] = -plane.c();
    clip_plane[3] = -plane.d();


    //The facets
    {
      for (C3t3::Facet_iterator
        fit = c3t3().facets_begin(),
        end = c3t3().facets_end();
      fit != end; ++fit)
      {
        const Tr::Cell_handle& cell = fit->first;
        const int& index = fit->second;
        const Kernel::Point_3& pa = cell->vertex((index + 1) & 3)->point();
        const Kernel::Point_3& pb = cell->vertex((index + 2) & 3)->point();
        const Kernel::Point_3& pc = cell->vertex((index + 3) & 3)->point();
        typedef Kernel::Oriented_side Side;
        using CGAL::ON_ORIENTED_BOUNDARY;
        const Side sa = plane.oriented_side(pa);
        const Side sb = plane.oriented_side(pb);
        const Side sc = plane.oriented_side(pc);
        bool is_showned = false;
        if (pa.x() * clip_plane[0] + pa.y() * clip_plane[1] + pa.z() * clip_plane[2] + clip_plane[3]  > 0
          && pb.x() * clip_plane[0] + pb.y() * clip_plane[1] + pb.z() * clip_plane[2] + clip_plane[3]  > 0
          && pc.x() * clip_plane[0] + pc.y() * clip_plane[1] + pc.z() * clip_plane[2] + clip_plane[3]  > 0)
          is_showned = true;

        if (is_showned && sa != ON_ORIENTED_BOUNDARY &&
          sb != ON_ORIENTED_BOUNDARY &&
          sc != ON_ORIENTED_BOUNDARY &&
          sb == sa && sc == sa)
        {
          if ((index % 2 == 1) == c3t3().is_in_complex(cell)) draw_triangle(pb, pa, pc, false);
          else draw_triangle(pa, pb, pc, false);
          draw_triangle_edges(pa, pb, pc);
        }

      }


      for (Tr::Finite_cells_iterator
        cit = c3t3().triangulation().finite_cells_begin(),
        end = c3t3().triangulation().finite_cells_end();
      cit != end; ++cit)
      {
        if (!c3t3().is_in_complex(cit))
          continue;

        const Kernel::Point_3& pa = cit->vertex(0)->point();
        const Kernel::Point_3& pb = cit->vertex(1)->point();
        const Kernel::Point_3& pc = cit->vertex(2)->point();
        const Kernel::Point_3& pd = cit->vertex(3)->point();
        typedef Kernel::Oriented_side Side;
        using CGAL::ON_ORIENTED_BOUNDARY;
        const Side sa = plane.oriented_side(pa);
        const Side sb = plane.oriented_side(pb);
        const Side sc = plane.oriented_side(pc);
        const Side sd = plane.oriented_side(pd);

        if (sa == ON_ORIENTED_BOUNDARY ||
          sb == ON_ORIENTED_BOUNDARY ||
          sc == ON_ORIENTED_BOUNDARY ||
          sd == ON_ORIENTED_BOUNDARY ||
          sb != sa || sc != sa || sd != sa)
        {
          draw_triangle(pb, pa, pc, true);
          draw_triangle(pa, pb, pd, true);
          draw_triangle(pa, pd, pc, true);
          draw_triangle(pb, pc, pd, true);

          draw_triangle_edges(pa, pb, pc);
          draw_triangle_edges(pa, pb, pd);
          draw_triangle_edges(pa, pc, pd);
          draw_triangle_edges(pb, pc, pd);
        }
      }
    }
  }

};








#endif // SCENE_C3T3_ITEM_H
