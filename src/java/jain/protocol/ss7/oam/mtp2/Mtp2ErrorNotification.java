/*
 @(#) src/java/jain/protocol/ss7/oam/mtp2/Mtp2ErrorNotification.java <p>
 
 Copyright &copy; 2008-2015  Monavacon Limited <a href="http://www.monavacon.com/">&lt;http://www.monavacon.com/&gt;</a>. <br>
 Copyright &copy; 2001-2008  OpenSS7 Corporation <a href="http://www.openss7.com/">&lt;http://www.openss7.com/&gt;</a>. <br>
 Copyright &copy; 1997-2001  Brian F. G. Bidulock <a href="mailto:bidulock@openss7.org">&lt;bidulock@openss7.org&gt;</a>. <p>
 
 All Rights Reserved. <p>
 
 This program is free software: you can redistribute it and/or modify it under
 the terms of the GNU Affero General Public License as published by the Free
 Software Foundation, version 3 of the license. <p>
 
 This program is distributed in the hope that it will be useful, but <b>WITHOUT
 ANY WARRANTY</b>; without even the implied warranty of <b>MERCHANTABILITY</b>
 or <b>FITNESS FOR A PARTICULAR PURPOSE</b>.  See the GNU Affero General Public
 License for more details. <p>
 
 You should have received a copy of the GNU Affero General Public License along
 with this program.  If not, see
 <a href="http://www.gnu.org/licenses/">&lt;http://www.gnu.org/licenses/&gt</a>,
 or write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA
 02139, USA. <p>
 
 <em>U.S. GOVERNMENT RESTRICTED RIGHTS</em>.  If you are licensing this
 Software on behalf of the U.S. Government ("Government"), the following
 provisions apply to you.  If the Software is supplied by the Department of
 Defense ("DoD"), it is classified as "Commercial Computer Software" under
 paragraph 252.227-7014 of the DoD Supplement to the Federal Acquisition
 Regulations ("DFARS") (or any successor regulations) and the Government is
 acquiring only the license rights granted herein (the license rights
 customarily provided to non-Government users).  If the Software is supplied to
 any unit or agency of the Government other than DoD, it is classified as
 "Restricted Computer Software" and the Government's rights in the Software are
 defined in paragraph 52.227-19 of the Federal Acquisition Regulations ("FAR")
 (or any successor regulations) or, in the cases of NASA, in paragraph
 18.52.227-86 of the NASA Supplement to the FAR (or any successor regulations). <p>
 
 Commercial licensing and support of this software is available from OpenSS7
 Corporation at a fee.  See
 <a href="http://www.openss7.com/">http://www.openss7.com/</a>
 */

package jain.protocol.ss7.oam.mtp2;

import javax.management.*;

import jain.protocol.ss7.oam.*;
import jain.protocol.ss7.*;
import jain.*;

/**
  * This Notification may be emitted by any MTP2 Managed MBean if a user defined error is
  * encountered. <p>
  *
  * This Mtp2ErrorNotification should be sent to all applications that have registered
  * with the Mtp2 MBean as a javax.management.NotificationListener using a filter 'f',
  * where f.isNotificationEnabled(this.getType()) == true. <p>
  *
  * The Notification class extends the java.util.EventObject base class and defines the
  * minimal information contained in a notification. It contains the following fields:
  * <ul>
  *
  * <li>the <em>notification type</em>, which is a string expressed in a dot notation
  * similar to Java properties.
  *
  * <li>a <em>sequence number</em>, which is a serial number identifying a particular
  * instance of notification in the context of the notification source
  *
  * <li>a <em>time stamp</em>, indicating when the notification was generated
  *
  * <li>a <em>message</em> contained in a string, which could be the explanation of the
  * notification for displaying to a user
  *
  * <li><em>userData</em> is used for whatever other data the notification source wishes
  * to communicate to its consumers </ul>
  *
  * Notification sources should use the notification type to indicate the nature of the
  * event to their consumers. When additional information needs to be transmitted to
  * consumers, the source may place it in the message or user data fields.
  *
  * <center> [NotificationHierarchy.jpg] </center><br>
  * <center> Inheritance hierarchy for JAIN OAM Notification </center><br>
  *
  * @author Monavacon Limited
  * @version 1.2.2
  */
public class Mtp2ErrorNotification extends OamErrorNotification {
    /**
      * Alarm Type Constant: Invalid MTP2 Timer Expired.
      * <br>Emitted By: Mtp2TimerProfileMBean
      * <br>Notification Type: "jain.protocol.ss7.oam.mtp2.error.invalid_timer_expired".
      */
    public static final int MTP2_ERROR_INVALID_TIMER_EXPIRED = 1;
    /**
      * Constructs a new Mtp2ErrorNotification of the specified Error Type.
      *
      * @param source
      * The source of the error.
      *
      * @param sequenceNumber
      * The notification sequence number within the source object.
      *
      * @param errorType
      * One of the defined Error Types.
      *
      * @exception IllegalArgumentException
      * If any of the supplied parameters are invalid
      */
    public Mtp2ErrorNotification(OamManagedObjectMBean source, long sequenceNumber, int errorType)
        throws IllegalArgumentException {
    }
    /**
      * Returns a string representation (with details) of classes which extend this class.
      * Over rides standard JAVA toString method.
      */
    public java.lang.String toString() {
        return new java.lang.String("");
    }
}

// vim: sw=4 et tw=72 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS,\://,b\:#,\:%,\:XCOMM,n\:>,fb\:-
