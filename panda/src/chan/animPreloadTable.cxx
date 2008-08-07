// Filename: animPreloadTable.cxx
// Created by:  drose (05Aug08)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#include "animPreloadTable.h"

#include "indent.h"
#include "datagram.h"
#include "datagramIterator.h"
#include "bamReader.h"
#include "bamWriter.h"

TypeHandle AnimPreloadTable::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: AnimPreloadTable::make_cow_copy
//       Access: Protected, Virtual
//  Description: Required to implement CopyOnWriteObject.
////////////////////////////////////////////////////////////////////
PT(CopyOnWriteObject) AnimPreloadTable::
make_cow_copy() {
  return new AnimPreloadTable(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: AnimPreloadTable::Constructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
AnimPreloadTable::
AnimPreloadTable() {
  _needs_sort = false;
}

////////////////////////////////////////////////////////////////////
//     Function: AnimPreloadTable::Destructor
//       Access: Published, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
AnimPreloadTable::
~AnimPreloadTable() {
}

////////////////////////////////////////////////////////////////////
//     Function: AnimPreloadTable::get_num_anims
//       Access: Published
//  Description: Returns the number of animation records in the table.
////////////////////////////////////////////////////////////////////
int AnimPreloadTable::
get_num_anims() const {
  return (int)_anims.size();
}

////////////////////////////////////////////////////////////////////
//     Function: AnimPreloadTable::find_anim
//       Access: Published
//  Description: Returns the index number in the table of the
//               animation record with the indicated name, or -1 if
//               the name is not present.  By convention, the basename
//               is the filename of the egg or bam file, without the
//               directory part and without the extension.  That is,
//               it is Filename::get_basename_wo_extension().
////////////////////////////////////////////////////////////////////
int AnimPreloadTable::
find_anim(const string &basename) const {
  consider_sort();
  AnimRecord record;
  record._basename = basename;
  Anims::const_iterator ai = _anims.find(record);
  if (ai != _anims.end()) {
    return int(ai - _anims.begin());
  }
  return -1;
}

////////////////////////////////////////////////////////////////////
//     Function: AnimPreloadTable::clear_anims
//       Access: Published
//  Description: Removes all animation records from the table.
////////////////////////////////////////////////////////////////////
void AnimPreloadTable::
clear_anims() {
  _anims.clear();
  _needs_sort = false;
}

////////////////////////////////////////////////////////////////////
//     Function: AnimPreloadTable::remove_anim
//       Access: Published
//  Description: Removes the nth animation records from the table.
//               This renumbers indexes for following animations.
////////////////////////////////////////////////////////////////////
void AnimPreloadTable::
remove_anim(int n) {
  nassertv(n >= 0 && n < (int)_anims.size());
  _anims.erase(_anims.begin() + n);
}

////////////////////////////////////////////////////////////////////
//     Function: AnimPreloadTable::add_anim
//       Access: Published
//  Description: Adds a new animation record to the table.  If there
//               is already a record of this name, no operation is
//               performed (the original record is unchanged).  See
//               find_anim().  This will invalidate existing index
//               numbers.
////////////////////////////////////////////////////////////////////
void AnimPreloadTable::
add_anim(const string &basename, float base_frame_rate, int num_frames) {
  AnimRecord record;
  record._basename = basename;
  record._base_frame_rate = base_frame_rate;
  record._num_frames = num_frames;

  _anims.push_back(record);
  _needs_sort = true;
}

////////////////////////////////////////////////////////////////////
//     Function: AnimPreloadTable::add_anims_from
//       Access: Published
//  Description: Copies the animation records from the other table
//               into this one.  If a given record name exists in both
//               tables, the record in this one supercedes.
////////////////////////////////////////////////////////////////////
void AnimPreloadTable::
add_anims_from(const AnimPreloadTable *other) {
  _anims.reserve(_anims.size() + other->_anims.size());
  Anims::const_iterator ai;
  for (ai = other->_anims.begin(); ai != other->_anims.end(); ++ai) {
    _anims.push_back(*ai);
  }
  _needs_sort = true;
}

////////////////////////////////////////////////////////////////////
//     Function: AnimPreloadTable::output
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
void AnimPreloadTable::
output(ostream &out) const {
  out << "AnimPreloadTable, " << _anims.size() << " animation records.";
}

////////////////////////////////////////////////////////////////////
//     Function: AnimPreloadTable::write
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
void AnimPreloadTable::
write(ostream &out, int indent_level) const {
  indent(out, indent_level)
    << "AnimPreloadTable, " << _anims.size() << " animation records:\n";
  consider_sort();
  Anims::const_iterator ai;
  for (ai = _anims.begin(); ai != _anims.end(); ++ai) {
    const AnimRecord &record = (*ai);
    indent(out, indent_level + 2)
      << record._basename << ": " << record._num_frames << " frames at "
      << record._base_frame_rate << " fps\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: AnimPreloadTable::register_with_read_factory
//       Access: Public, Static
//  Description: Factory method to generate an AnimPreloadTable object
////////////////////////////////////////////////////////////////////
void AnimPreloadTable::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_from_bam);
}

////////////////////////////////////////////////////////////////////
//     Function: AnimPreloadTable::write_datagram
//       Access: Public
//  Description: Function to write the important information in
//               the particular object to a Datagram
////////////////////////////////////////////////////////////////////
void AnimPreloadTable::
write_datagram(BamWriter *manager, Datagram &dg) {
  consider_sort();

  dg.add_uint16(_anims.size());
  Anims::const_iterator ai;
  for (ai = _anims.begin(); ai != _anims.end(); ++ai) {
    const AnimRecord &record = (*ai);
    dg.add_string(record._basename);
    dg.add_float32(record._base_frame_rate);
    dg.add_int32(record._num_frames);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: AnimPreloadTable::make_from_bam
//       Access: Protected
//  Description: Factory method to generate an AnimPreloadTable object
////////////////////////////////////////////////////////////////////
TypedWritable *AnimPreloadTable::
make_from_bam(const FactoryParams &params) {
  AnimPreloadTable *me = new AnimPreloadTable;
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  me->fillin(scan, manager);
  return me;
}

////////////////////////////////////////////////////////////////////
//     Function: AnimPreloadTable::fillin
//       Access: Protected
//  Description: Function that reads out of the datagram (or asks
//               manager to read) all of the data that is needed to
//               re-create this object and stores it in the appropiate
//               place
////////////////////////////////////////////////////////////////////
void AnimPreloadTable::
fillin(DatagramIterator &scan, BamReader *manager) {
  int num_anims = scan.get_uint16();
  _anims.reserve(num_anims);
  for (int i = 0; i < num_anims; ++i) {
    AnimRecord record;
    record._basename = scan.get_string();
    record._base_frame_rate = scan.get_float32();
    record._num_frames = scan.get_int32();
    _anims.push_back(record);
  }
}
